#include "../include/CmdHelpers.hpp"
#include <map>
#include <string>

namespace CmdHelpers
{

// ---------------------------------------------------------------------------
// getChannelOrError
// ---------------------------------------------------------------------------
Channel *getChannelOrError(Server &server, Client &caller,
                           const std::string &channelName)
{
    std::map<std::string, Channel> &channels = server.getChannels();
    std::map<std::string, Channel>::iterator it = channels.find(channelName);
    if (it == channels.end())
    {
        server.sendError(caller, ERR_NOSUCHCHANNEL,
                         channelName + " :No such channel");
        return NULL;
    }
    return &it->second;
}

// ---------------------------------------------------------------------------
// getClientByNick
// ---------------------------------------------------------------------------
Client *getClientByNick(Server &server, Client &caller,
                        const std::string &nick)
{
    std::map<int, Client> &clients = server.getClients();
    for (std::map<int, Client>::iterator it = clients.begin();
         it != clients.end(); ++it)
    {
        if (it->second.getNickname() == nick)
            return &it->second;
    }
    server.sendError(caller, ERR_NOSUCHNICK, nick + " :No such nick/channel");
    return NULL;
}

// ---------------------------------------------------------------------------
// requireMember  — checks that the CALLER is in the channel
// ---------------------------------------------------------------------------
bool requireMember(Server &server, Client &caller, Channel &channel)
{
    if (!channel.hasMember(caller.getFd()))
    {
        server.sendError(caller, ERR_NOTONCHANNEL,
                         channel.getName() + " :You're not on that channel");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// requireOperator — checks that the CALLER is a channel operator
// ---------------------------------------------------------------------------
bool requireOperator(Server &server, Client &caller, Channel &channel)
{
    if (!channel.isOperator(caller.getFd()))
    {
        server.sendError(caller, ERR_CHANOPRIVSNEEDED,
                         channel.getName() + " :You're not channel operator");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// requireTargetMember — checks that the TARGET is in the channel
// ---------------------------------------------------------------------------
bool requireTargetMember(Server &server, Client &caller,
                         Channel &channel, Client &target)
{
    if (!channel.hasMember(target.getFd()))
    {
        server.sendError(caller, ERR_NOTONCHANNEL,
                         target.getNickname() + " " + channel.getName() +
                             " :They aren't on that channel");
        return false;
    }
    return true;
}

} // namespace CmdHelpers
