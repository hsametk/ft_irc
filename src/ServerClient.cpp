#include "../include/Channel.hpp"
#include "../include/Server.hpp"

// ---------------------------------------------------------------------------
// ServerClient — client yaşam döngüsü: poll setinden çıkarma, kanallardan
// temizleme, soket kapatma ve nick ile client bulma.
// ---------------------------------------------------------------------------

// Poll setinden ilgili fd'yi çıkarır.
void Server::removePollFd(int fd)
{
    for (size_t i = 0; i < _pfds.size(); ++i)
    {
        if (_pfds[i].fd == fd)
        {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }
}

// Client'ı bulunduğu tüm kanallardan çıkarır, diğer üyelere QUIT bildirir,
// boşalan kanalı siler.
void Server::notifyChannelsOnQuit(int fd)
{
    std::map<int, Client>::iterator clientIt = _clients.find(fd);
    if (clientIt == _clients.end())
        return;

    Client &client = clientIt->second;
    std::string prefix =
        client.getNickname() + "!" + client.getUsername() + "@localhost";
    std::string quitMsg = ":" + prefix + " QUIT :connection closed\r\n";

    std::map<std::string, Channel>::iterator ch = _channels.begin();
    while (ch != _channels.end())
    {
        if (ch->second.hasMember(fd))
        {
            ch->second.broadcast(quitMsg, fd);
            ch->second.removeMember(fd);
            if (ch->second.getMembers().empty())
            {
                std::cout << "Channel " << ch->first << " is now empty, removing."
                          << std::endl;
                _channels.erase(ch++);
                continue;
            }
        }
        ++ch;
    }
}

// Bağlantı kapanınca tüm temizliği yapar.
void Server::removeClient(int fd)
{
    removePollFd(fd);
    notifyChannelsOnQuit(fd);
    close(fd);
    _clients.erase(fd);
    std::cout << "Client removed: fd=" << fd << std::endl;
}

// Kanal boşaldıysa map'ten siler.
void Server::removeChannelIfEmpty(const std::string &channelName)
{
    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    if (it != _channels.end() && it->second.getMembers().empty())
    {
        std::cout << "Channel " << channelName << " is now empty, removing." << std::endl;
        _channels.erase(it);
    }
}

// Verilen nick'e sahip client'ı bulur (case-sensitive). Yoksa NULL döner.
Client *Server::findClientByNick(const std::string &nick)
{
    for (std::map<int, Client>::iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        if (it->second.getNickname() == nick)
            return &it->second;
    }
    return NULL;
}