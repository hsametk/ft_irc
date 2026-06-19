#include "../include/CmdHelpers.hpp"

// ---------------------------------------------------------------------------
// CmdHelpers_getChannelOrError
// ---------------------------------------------------------------------------
Channel *CmdHelpers_getChannelOrError(Server &server, Client &caller,
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
// CmdHelpers_getClientByNick
// ---------------------------------------------------------------------------
Client *CmdHelpers_getClientByNick(Server &server, Client &caller,
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
// CmdHelpers_requireMember  — checks that the CALLER is in the channel
// ---------------------------------------------------------------------------
bool CmdHelpers_requireMember(Server &server, Client &caller, Channel &channel)
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
// CmdHelpers_requireOperator — checks that the CALLER is a channel operator
// ---------------------------------------------------------------------------
bool CmdHelpers_requireOperator(Server &server, Client &caller, Channel &channel)
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
// CmdHelpers_requireTargetMember — checks that the TARGET is in the channel
// ---------------------------------------------------------------------------
bool CmdHelpers_requireTargetMember(Server &server, Client &caller,
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
