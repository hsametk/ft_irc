#include "../include/Channel.hpp"
#include "../include/Server.hpp"

// ---------------------------------------------------------------------------
// ServerChannel — kanal üyelik komutları (JOIN, PART, NAMES) ve komut
// parametre ayrıştırıcıları.
// ---------------------------------------------------------------------------

// --- Parse: sondaki \r ve boşluk karakterlerini siler ---
void Server::trimTrailing(std::string &s)
{
    while (!s.empty() && (s[s.size() - 1] == '\r' || s[s.size() - 1] == ' '))
        s.erase(s.size() - 1);
}

// --- Parse: "#kanal" veya "#kanal :neden" → kanal adı + sebep ---
void Server::parseChannelAndReason(const std::string &params,
                                   std::string &channelName, std::string &reason)
{
    size_t spacePos = params.find(' ');
    if (spacePos != std::string::npos)
    {
        channelName = params.substr(0, spacePos);
        reason = params.substr(spacePos + 1);
        if (!reason.empty() && reason[0] == ':')
            reason = reason.substr(1);
    }
    else
    {
        channelName = params;
        reason.clear();
    }
    trimTrailing(channelName);
}

// --- Parse: TOPIC parametreleri (kanal + varsa yeni topic) ---
void Server::parseTopicParams(const std::string &params, std::string &channelName,
                              std::string &newTopic, bool &topicGiven)
{
    size_t spacePos = params.find(' ');
    if (spacePos != std::string::npos)
    {
        channelName = params.substr(0, spacePos);
        newTopic = params.substr(spacePos + 1);
        topicGiven = true;
        if (!newTopic.empty() && newTopic[0] == ':')
            newTopic = newTopic.substr(1);
        trimTrailing(newTopic);
    }
    else
    {
        channelName = params;
        newTopic.clear();
        topicGiven = false;
    }
    trimTrailing(channelName);
}

// --- Parse: INVITE parametreleri (<nick> <channel>) ---
void Server::parseInviteParams(const std::string &params,
                               std::string &nickArg, std::string &channelArg)
{
    size_t spacePos = params.find(' ');
    if (spacePos == std::string::npos)
    {
        nickArg = params;
        channelArg.clear();
    }
    else
    {
        nickArg = params.substr(0, spacePos);
        channelArg = params.substr(spacePos + 1);
        while (!channelArg.empty() && channelArg[0] == ' ')
            channelArg.erase(0, 1);
        size_t nextSpace = channelArg.find(' ');
        if (nextSpace != std::string::npos)
            channelArg = channelArg.substr(0, nextSpace);
    }
    trimTrailing(nickArg);
    trimTrailing(channelArg);
}

// --- JOIN: katılım kontrolleri (+k / +i / +l / zaten üye) ---
bool Server::canClientJoin(Client &client, Channel &channel,
                           const std::string &key, const std::string &channelName)
{
    if (channel.hasMember(client.getFd()))
    {
        sendError(client, ERR_USERONCHANNEL,
                  client.getNickname() + " " + channelName + " :is already on channel");
        return false;
    }
    if (!channel.getKey().empty() && channel.getKey() != key)
    {
        sendError(client, ERR_BADCHANNELKEY, channelName + " :Cannot join channel (+k)");
        return false;
    }
    if (channel.isInviteOnly() && !channel.isInvited(client.getFd()))
    {
        sendError(client, ERR_INVITEONLYCHAN, channelName + " :Cannot join channel (+i)");
        return false;
    }
    if (channel.getLimit() > 0 &&
        (int)channel.getMembers().size() >= channel.getLimit())
    {
        sendError(client, ERR_CHANNELISFULL, channelName + " :Cannot join channel (+l)");
        return false;
    }
    return true;
}

// --- JOIN: yanıtlar (JOIN mesajı, topic 331/332, NAMES, kanala duyuru) ---
void Server::sendJoinReplies(Client &client, const std::string &channelName,
                             Channel &channel)
{
    std::string prefix =
        client.getNickname() + "!" + client.getUsername() + "@localhost";
    std::string joinMsg = ":" + prefix + " JOIN " + channelName + "\r\n";
    client.sendMessage(joinMsg);

    if (!channel.getTopic().empty())
        client.sendMessage(":ircserv 332 " + client.getNickname() + " " +
                           channelName + " :" + channel.getTopic() + "\r\n");
    else
        client.sendMessage(":ircserv 331 " + client.getNickname() + " " +
                           channelName + " :No topic is set\r\n");

    sendNamesList(client, channel);
    channel.broadcast(joinMsg, client.getFd());
}

// --- JOIN ---
void Server::joinChannel(Client &client, const std::string &params)
{
    std::string channelName;
    std::string key;
    size_t spacePos = params.find(' ');
    if (spacePos != std::string::npos)
    {
        channelName = params.substr(0, spacePos);
        key = params.substr(spacePos + 1);
    }
    else
        channelName = params;

    if (channelName.empty() || channelName[0] != '#')
    {
        sendError(client, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
        return;
    }

    // Kanal yoksa oluştur; oluşturan kişi operator olur.
    bool isNewChannel = (_channels.find(channelName) == _channels.end());
    if (isNewChannel)
        _channels[channelName] = Channel(channelName);
    Channel &channel = _channels[channelName];

    if (!canClientJoin(client, channel, key, channelName))
        return;

    channel.addMember(&client, isNewChannel);
    channel.removeInvited(client.getFd());
    sendJoinReplies(client, channelName, channel);
}

// --- PART ---
void Server::partChannel(Client &client, const std::string &params)
{
    std::string channelName;
    std::string reason;
    parseChannelAndReason(params, channelName, reason);

    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    if (it == _channels.end())
    {
        sendError(client, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
        return;
    }
    if (!it->second.hasMember(client.getFd()))
    {
        client.sendMessage(":ircserv 442 " + client.getNickname() + " " +
                           channelName + " :You're not on that channel\r\n");
        return;
    }

    std::string prefix =
        client.getNickname() + "!" + client.getUsername() + "@localhost";
    std::string partMsg = ":" + prefix + " PART " + channelName;
    if (!reason.empty())
        partMsg += " :" + reason;
    partMsg += "\r\n";

    client.sendMessage(partMsg);
    it->second.broadcast(partMsg, client.getFd());
    it->second.removeMember(client.getFd());
    std::cout << "PART: " << client.getNickname() << " left " << channelName << std::endl;

    removeChannelIfEmpty(channelName);
}

// --- NAMES ---
// JOIN sonrası RPL_NAMREPLY (353) ve RPL_ENDOFNAMES (366) gönderir.
void Server::sendNamesList(Client &client, Channel &channel)
{
    std::string names;
    const std::map<int, Client *> &members = channel.getMembers();

    for (std::map<int, Client *>::const_iterator it = members.begin();
         it != members.end(); ++it)
    {
        if (!names.empty())
            names += " ";
        if (channel.isOperator(it->first)) // Operator'ler @ prefix'i ile gösterilir.
            names += "@";
        names += it->second->getNickname();
    }

    client.sendMessage(":ircserv 353 " + client.getNickname() + " = " +
                       channel.getName() + " :" + names + "\r\n");
    client.sendMessage(":ircserv 366 " + client.getNickname() + " " +
                       channel.getName() + " :End of /NAMES list\r\n");
}
