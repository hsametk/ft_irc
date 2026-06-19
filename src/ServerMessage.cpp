#include "../include/Channel.hpp"
#include "../include/Server.hpp"

// ---------------------------------------------------------------------------
// ServerMessage — mesajlaşma ve operatör komutları: PRIVMSG, TOPIC, INVITE.
// ---------------------------------------------------------------------------

// --- PRIVMSG: kanala ---
void Server::sendToChannel(Client &client, const std::string &target,
                           const std::string &fullMsg)
{
    std::map<std::string, Channel>::iterator it = _channels.find(target);
    if (it == _channels.end())
    {
        sendError(client, 403, target + " :No such channel");
        return;
    }
    if (!it->second.hasMember(client.getFd()))
    {
        sendError(client, 404, target + " :Cannot send to channel");
        return;
    }
    it->second.broadcast(fullMsg, client.getFd());
}

// --- PRIVMSG: kullanıcıya ---
void Server::sendToUser(Client &client, const std::string &target,
                        const std::string &fullMsg)
{
    for (std::map<int, Client>::iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        if (it->second.getNickname() == target)
        {
            it->second.sendMessage(fullMsg);
            return;
        }
    }
    sendError(client, 401, target + " :No such nick/channel");
}

void Server::sendServerMessage(Client &client, const std::string &params)
{
    if (params.empty())
    {
        sendError(client, 411, ":No recipient given (PRIVMSG)");
        return;
    }
    size_t spacePos = params.find(' ');
    if (spacePos == std::string::npos)
    {
        sendError(client, 412, ":No text to send");
        return;
    }

    std::string target = params.substr(0, spacePos);
    std::string message = params.substr(spacePos + 1);
    if (message.empty() || (message[0] == ':' && message.size() == 1))
    {
        sendError(client, 412, ":No text to send");
        return;
    }
    if (message[0] == ':')
        message = message.substr(1);

    std::string fullMsg = ":" + client.getNickname() + "!" +
        client.getUsername() + "@localhost PRIVMSG " + target + " :" + message + "\r\n";

    if (target[0] == '#')
        sendToChannel(client, target, fullMsg);
    else
        sendToUser(client, target, fullMsg);
}

// --- TOPIC: görüntüleme (331/332) ---
void Server::sendTopicView(Client &client, const std::string &channelName,
                           Channel &channel)
{
    if (channel.getTopic().empty())
        client.sendMessage(":ircserv 331 " + client.getNickname() + " " +
                           channelName + " :No topic is set\r\n");
    else
        client.sendMessage(":ircserv 332 " + client.getNickname() + " " +
                           channelName + " :" + channel.getTopic() + "\r\n");
}

// --- TOPIC: değiştirme (sadece operator) ---
void Server::setChannelTopic(Client &client, const std::string &channelName,
                             Channel &channel, const std::string &newTopic)
{
    if (!channel.isOperator(client.getFd()))
    {
        client.sendMessage(":ircserv 482 " + client.getNickname() + " " +
                           channelName + " :You're not channel operator\r\n");
        return;
    }
    channel.setTopic(newTopic);

    std::string prefix =
        client.getNickname() + "!" + client.getUsername() + "@localhost";
    std::string topicMsg = ":" + prefix + " TOPIC " + channelName + " :" + newTopic + "\r\n";
    client.sendMessage(topicMsg);
    channel.broadcast(topicMsg, client.getFd());
}

void Server::topicCommand(Client &client, const std::string &params)
{
    std::string channelName;
    std::string newTopic;
    bool topicGiven = false;
    parseTopicParams(params, channelName, newTopic, topicGiven);

    if (channelName.empty())
    {
        client.sendMessage(":ircserv 461 " + client.getNickname() +
                           " TOPIC :Not enough parameters\r\n");
        return;
    }
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

    if (!topicGiven)
        sendTopicView(client, channelName, it->second);
    else
        setChannelTopic(client, channelName, it->second, newTopic);
}

// --- INVITE: başarı işlemi (invite list'e ekle, 341, hedefe INVITE) ---
void Server::sendInviteSuccess(Client &client, Client &target,
                               const std::string &nickArg, const std::string &channelArg,
                               Channel &channel)
{
    channel.addInvited(target.getFd());

    client.sendMessage(":ircserv 341 " + client.getNickname() +
                       " " + nickArg + " " + channelArg + "\r\n");

    std::string prefix =
        client.getNickname() + "!" + client.getUsername() + "@localhost";
    std::string inviteMsg = ":" + prefix + " INVITE " + nickArg + " :" + channelArg + "\r\n";
    target.sendMessage(inviteMsg);

    std::cout << "INVITE: " << client.getNickname() << " invited "
              << nickArg << " to " << channelArg << std::endl;
}

// --- INVITE ---
// Kontrol sırası: 461 -> 403 -> 442 -> 482 -> 401 -> 443
void Server::inviteCommand(Client &client, const std::string &params)
{
    std::string nickArg;
    std::string channelArg;
    parseInviteParams(params, nickArg, channelArg);

    if (nickArg.empty() || channelArg.empty())
    {
        client.sendMessage(":ircserv 461 " + client.getNickname() +
                           " INVITE :Not enough parameters\r\n");
        return;
    }
    std::map<std::string, Channel>::iterator it = _channels.find(channelArg);
    if (it == _channels.end())
    {
        sendError(client, ERR_NOSUCHCHANNEL, channelArg + " :No such channel");
        return;
    }
    if (!it->second.hasMember(client.getFd()))
    {
        client.sendMessage(":ircserv 442 " + client.getNickname() + " " +
                           channelArg + " :You're not on that channel\r\n");
        return;
    }
    if (!it->second.isOperator(client.getFd()))
    {
        client.sendMessage(":ircserv 482 " + client.getNickname() + " " +
                           channelArg + " :You're not channel operator\r\n");
        return;
    }
    Client *target = findClientByNick(nickArg);
    if (target == NULL)
    {
        client.sendMessage(":ircserv 401 " + client.getNickname() + " " +
                           nickArg + " :No such nick\r\n");
        return;
    }
    if (it->second.hasMember(target->getFd()))
    {
        client.sendMessage(":ircserv 443 " + client.getNickname() + " " +
                           nickArg + " " + channelArg + " :is already on channel\r\n");
        return;
    }

    sendInviteSuccess(client, *target, nickArg, channelArg, it->second);
}