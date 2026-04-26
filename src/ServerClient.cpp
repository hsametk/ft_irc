#include "../include/Server.hpp"
#include "../include/Channel.hpp"

// Private: Remove Client
void Server::removeClient(int fd)
{
    // Poll setinden çıkar.
    for (size_t i = 0; i < _pfds.size(); ++i)
    {
        if (_pfds[i].fd == fd)
        {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }

    // Tüm kanallardan bu client'ı çıkar.
    // Client quit/disconnect ederken kanaldaki diğer üyelere bildir.
    std::map<int, Client>::iterator clientIt = _clients.find(fd);
    if (clientIt != _clients.end())
    {
        Client& client = clientIt->second;
        std::string prefix = client.getNickname() + "!" + client.getUsername() + "@localhost";
        std::string quitMsg = ":" + prefix + " QUIT :connection closed\r\n";

        for (std::map<std::string, Channel>::iterator ch = _channels.begin();
             ch != _channels.end(); ++ch)
        {
            if (ch->second.hasMember(fd))
            {
                // Kanaldaki diğer üyelere QUIT bildir, ardından üyeyi sil.
                ch->second.broadcast(quitMsg, fd);
                ch->second.removeMember(fd);
            }
        }
    }

    // Soketi kapat ve clients map'inden sil.
    close(fd);
    _clients.erase(fd);
    std::cout << "Client removed: fd=" << fd << std::endl;
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
    {
        channelName = params;
    }

    // Channel name must start with '#'
    if (channelName.empty() || channelName[0] != '#')
    {
        sendError(client, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
        return;
    }

    // Check if channel exists
    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    if (it == _channels.end())
    {
        // Create new channel
        _channels[channelName] = Channel(channelName);
        it = _channels.find(channelName);
    }

    // Check if client is already in channel
    if (it->second.hasMember(client.getFd()))
    {
        sendError(client, ERR_USERONCHANNEL, client.getNickname() + " " + channelName + " :is already on channel");
        return;
    }

    // Check channel key if required
    if (!it->second.getKey().empty() && it->second.getKey() != key)
    {
        sendError(client, ERR_BADCHANNELKEY, channelName + " :Cannot join channel (+k)");
        return;
    }

    // Check invite-only
    if (it->second.isInviteOnly())
    {
        sendError(client, ERR_INVITEONLYCHAN, channelName + " :Cannot join channel (+i)");
        return;
    }

    // Check user limit
    if (it->second.getLimit() > 0 && (int)it->second.getMembers().size() >= it->second.getLimit())
    {
        sendError(client, ERR_CHANNELISFULL, channelName + " :Cannot join channel (+l)");
        return;
    }

    // Add client to channel
    it->second.addMember(&client);

    // Send JOIN message to client
    std::string prefix = client.getNickname() + "!" + client.getUsername() + "@localhost"; // TODO: hostname
    std::string joinMsg = ":" + prefix + " JOIN " + channelName + "\r\n";
    send(client.getFd(), joinMsg.c_str(), joinMsg.size(), 0);

    // Send topic if exists
    if (!it->second.getTopic().empty())
    {
        std::string topicMsg = ":server 332 " + client.getNickname() + " " + channelName + " :" + it->second.getTopic() + "\r\n";
        send(client.getFd(), topicMsg.c_str(), topicMsg.size(), 0);
    }

    // Send NAMES list - TODO: implement sendNamesList
    // sendNamesList(client, it->second);

    // Notify other clients in channel
    std::string notifyMsg = ":" + prefix + " JOIN " + channelName + "\r\n";
    it->second.broadcast(notifyMsg, client.getFd());
}

// --- PART ---
void Server::partChannel(Client& client, const std::string& params)
{
    // params: "#kanal" veya "#kanal :neden"
    std::string channelName;
    std::string reason;

    size_t spacePos = params.find(' ');
    if (spacePos != std::string::npos)
    {
        channelName = params.substr(0, spacePos);
        reason = params.substr(spacePos + 1);
        // Trailing ':' kaldır
        if (!reason.empty() && reason[0] == ':')
            reason = reason.substr(1);
    }
    else
    {
        channelName = params;
        reason = "";
    }

    // Sondaki boşluk / CR temizle
    size_t end = channelName.size();
    while (end > 0 && (channelName[end - 1] == ' ' || channelName[end - 1] == '\r'))
        --end;
    channelName = channelName.substr(0, end);

    // Kanal mevcut mu?
    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    if (it == _channels.end())
    {
        sendError(client, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
        return;
    }

    // Client kanalda mı?
    if (!it->second.hasMember(client.getFd()))
    {
        // 442 ERR_NOTONCHANNEL
        std::string errMsg = ":ircserv 442 " + client.getNickname()
                            + " " + channelName + " :You're not on that channel\r\n";
        client.sendMessage(errMsg);
        return;
    }

    // Kanaldaki herkese (ayrılan dahil) PART bildir.
    std::string prefix = client.getNickname() + "!" + client.getUsername() + "@localhost";
    std::string partMsg = ":" + prefix + " PART " + channelName;
    if (!reason.empty())
        partMsg += " :" + reason;
    partMsg += "\r\n";

    // Ayrılan kişiye de gönder, sonra kanaldan çıkar.
    client.sendMessage(partMsg);
    it->second.broadcast(partMsg, client.getFd());
    it->second.removeMember(client.getFd());

    std::cout << "PART: " << client.getNickname() << " left " << channelName << std::endl;
}