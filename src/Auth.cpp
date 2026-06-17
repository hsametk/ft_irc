
#include "../include/Server.hpp"
#include "../include/Auth.hpp"

std::string normalize_spaces(const std::string &line) //zülal
{
    std::string result;
    bool inSpace = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == ' ' || line[i] == '\t' || line[i] == '\r' || line[i] == '\n')
        {
            if (!inSpace)
            {
                result += ' ';
                inSpace = true;
            }
        }
        else
        {
            result += line[i];
            inSpace = false;
        }
    }
    // Baştaki ve sondaki boşlukları kesin temizle
    size_t start = result.find_first_not_of(" ");
    if (start == std::string::npos) return "";
    size_t end = result.find_last_not_of(" ");
    return result.substr(start, end - start + 1);
}

ParsedCommand parse_line(const std::string &line) //zülal
{
    ParsedCommand parsed;
    std::string normalized = normalize_spaces(line);

    if (normalized.empty())
        return parsed;
    size_t pos = normalized.find(' ');
    if (pos == std::string::npos)
    {
        parsed.command = normalized;
        return parsed;
    }
    parsed.command = normalized.substr(0, pos);
    std::string rest = normalized.substr(pos + 1);

    while (!rest.empty())
    {
        if (rest[0] == ':')
        {
            parsed.args.push_back(rest.substr(1));
            break;
        }
        size_t space = rest.find(' ');
        if (space == std::string::npos)
        {
            parsed.args.push_back(rest);
            break;
        }
        parsed.args.push_back(rest.substr(0, space));
        rest = rest.substr(space + 1);
        // normalized_spaces zaten boşlukları temizlediği için 
        // peş peşe boşluk gelmeyecektir.
    }
    return parsed;
}

static bool isNickInUse(const std::map<int, Client> &clients,
                        const std::string &nick, int selfFd)
{
    std::string lowerNick = nick;
    for (size_t i = 0; i < lowerNick.size(); ++i)
        lowerNick[i] = std::tolower(lowerNick[i]);

    for (std::map<int, Client>::const_iterator it = clients.begin();
         it != clients.end(); ++it)
    {
        if (it->first != selfFd)
        {
            std::string existingNick = it->second.getNickname();
            std::string lowerExisting = existingNick;
            for (size_t i = 0; i < lowerExisting.size(); ++i)
                lowerExisting[i] = std::tolower(lowerExisting[i]);
            
            if (lowerExisting == lowerNick)
                return true;
        }
    }
    return false;
}

static void send_welcome(Client &client)
{
    const std::string &nick = client.getNickname();

    // 001-004: Standart kayit mesajlari
    client.sendMessage(":ft_irc 001 " + nick + " :Welcome to the ft_irc Network, " + nick + "\r\n");
    client.sendMessage(":ft_irc 002 " + nick + " :Your host is ft_irc, running version 1.0\r\n");
    client.sendMessage(":ft_irc 003 " + nick + " :This server was created Apr 2026\r\n");
    client.sendMessage(":ft_irc 004 " + nick + " ft_irc 1.0 o o\r\n");
    // MOTD
    client.sendMessage(":ft_irc 375 " + nick + " :- ft_irc Message of the Day -\r\n");
    client.sendMessage(":ft_irc 372 " + nick + " :- Welcome to your own IRC server\r\n");
    client.sendMessage(":ft_irc 372 " + nick + " :- This server is built as part of 42 ft_irc project\r\n");
    client.sendMessage(":ft_irc 372 " + nick + " :- Be respectful and have fun chatting!\r\n");
    client.sendMessage(":ft_irc 372 " + nick + " :- Available commands: JOIN, PRIVMSG, NICK, USER, TOPIC, INVITE\r\n");
    client.sendMessage(":ft_irc 372 " + nick + " :- Example: /join #42\r\n");
    client.sendMessage(":ft_irc 376 " + nick + " :End of MOTD\r\n");
}

void handle_pass(Client &client, const std::vector<std::string> &args,
                 const std::string &serverPassword)
{
    if (args.empty())
    {
        client.sendMessage(":ircserv 461 * PASS :Not enough parameters\r\n");
        return;
    }
    if (args[0] == serverPassword)
    {
        client.setPassOk(true);
        std::cout << "PASS accepted for fd=" << client.getFd() << std::endl;
    }
    else
    {
        client.sendMessage(":ircserv 464 * :Password incorrect\r\n");
        std::cerr << "Wrong password from fd=" << client.getFd() << std::endl;
    }
}

void handle_nick_registration(Client &client, const std::vector<std::string> &args,
                              const std::map<int, Client> &clients)
{
    if (args.empty())
    {
        client.sendMessage(":ircserv 431 * :No nickname given\r\n");
        return;
    }

    if (isNickInUse(clients, args[0], client.getFd()))
    {
        client.sendMessage(":ircserv 433 * " + args[0] + " :Nickname is already in use\r\n");
        std::cerr << "Nick '" << args[0] << "' already in use, rejected fd="
                  << client.getFd() << std::endl;
        return;
    }

    client.setNickname(args[0]);
    client.setNickSet(true);
    std::cout << "NICK set: " << args[0] << " for fd=" << client.getFd() << std::endl;
}

void handle_user(Client &client, const std::vector<std::string> &args)
{
    if (args.size() < 4)
    {
        client.sendMessage(":ircserv 461 * USER :Not enough parameters\r\n");
        return;
    }

    client.setUsername(args[0]);
    client.setUserSet(true);
    std::cout << "USER set: " << args[0] << std::endl;
}

void registration_state(Client &client, const std::string &line,
                        const std::string &serverPassword,
                        const std::map<int, Client> &clients)
{
    ParsedCommand parsed = parse_line(line);

    if (parsed.command.empty())
        return;
    std::string command = parsed.command;
    std::vector<std::string> args = parsed.args;
    // Debug: parser kontrolü
    std::cout << "[PARSED] CMD=" << command << std::endl;
    for (size_t i = 0; i < args.size(); ++i)
        std::cout << "[PARSED] ARG[" << i << "]=" << args[i] << std::endl;
    // CAP: irssi capability müzakeresi
    if (command == "CAP")
    {
        if (!args.empty() && args[0] == "LS")
            client.sendMessage(":ircserv CAP * LS :\r\n");
        return;
    }
    if (command == "PASS")
        handle_pass(client, args, serverPassword);
    else if (command == "NICK")
        handle_nick_registration(client, args, clients);
    else if (command == "USER")
        handle_user(client, args);
    else
    {
        // Kayıt tamamlanmadan gelen diğer komutlar için 451 ERR_NOTREGISTERED
        client.sendMessage(":ircserv 451 * :You have not registered\r\n");
        return;
    }
    // Üç koşul da sağlandı mı? -> kayıt tamamlandı
    if (client.isPassOk() && client.isNickSet() && client.isUserSet())
    {
        client.setRegistered(true);
        std::cout << "Client fd=" << client.getFd() << " is now registered!" << std::endl;
        send_welcome(client);
    }
}

bool handle_ping(Client &client, const std::vector<std::string> &args,
                 const std::string &params)
{
    if (args.empty())
        client.sendMessage(":ircserv 409 " + client.getNickname() + " :No origin specified\r\n");
    else
        client.sendMessage(":ircserv PONG ircserv :" + params + "\r\n");
    return false;
}

bool handle_quit(Client &client, const std::string &params)
{
    std::string reason = params.empty() ? "Client quit" : params;

    if (!reason.empty() && reason[0] == ':')
        reason = reason.substr(1);

    client.sendMessage(":ircserv ERROR :Closing connection (" + reason + ")\r\n");
    std::cout << "QUIT from fd=" << client.getFd()
              << " reason: " << reason << std::endl;
    return true;
}

bool handle_nick(Client &client, const std::vector<std::string> &args,
                 const std::map<int, Client> &clients)
{
    if (args.empty())
    {
        client.sendMessage(":ircserv 431 " + client.getNickname()
                           + " :No nickname given\r\n");
        return false;
    }

    if (isNickInUse(clients, args[0], client.getFd()))
    {
        client.sendMessage(":ircserv 433 " + client.getNickname()
                           + " " + args[0] + " :Nickname is already in use\r\n");
        return false;
    }

    std::string oldNick = client.getNickname();
    client.setNickname(args[0]);

    client.sendMessage(":" + oldNick + "!" + client.getUsername()
                       + "@localhost NICK :" + args[0] + "\r\n");

    return false;
}

bool handle_join(Client &client, const std::vector<std::string> &args,
                 Server &server)
{
    if (args.empty())
    {
        client.sendMessage(":ircserv 461 " + client.getNickname() + " JOIN :Not enough parameters\r\n");
        return false;
    }

    // Kanalları ve key'leri virgülle ayır.
    std::string channelList = args[0];
    std::string keyList = args.size() > 1 ? args[1] : "";

    std::vector<std::string> channels;
    std::vector<std::string> keys;

    // Kanalları parse et
    size_t pos = 0;
    while (pos < channelList.size())
    {
        size_t comma = channelList.find(',', pos);
        if (comma == std::string::npos)
        {
            channels.push_back(channelList.substr(pos));
            break;
        }
        channels.push_back(channelList.substr(pos, comma - pos));
        pos = comma + 1;
    }

    // Key'leri parse et
    pos = 0;
    while (pos < keyList.size())
    {
        size_t comma = keyList.find(',', pos);
        if (comma == std::string::npos)
        {
            keys.push_back(keyList.substr(pos));
            break;
        }
        keys.push_back(keyList.substr(pos, comma - pos));
        pos = comma + 1;
    }

    // Her kanal için joinChannel çağır
    for (size_t i = 0; i < channels.size(); ++i)
    {
        std::string singleParams = channels[i];
        if (i < keys.size() && !keys[i].empty())
            singleParams += " " + keys[i];
        server.joinChannel(client, singleParams);
    }
    return false;
}

bool handle_part(Client &client, const std::vector<std::string> &args,
                 const std::string &params, Server &server)
{
    if (args.empty())
    {
        client.sendMessage(":ircserv 461 " + client.getNickname() + " PART :Not enough parameters\r\n");
        return false;
    }
    server.partChannel(client, params);
    return false;
}

bool handle_topic(Client &client, const std::vector<std::string> &args,
                  const std::string &params, Server &server)
{
    if (args.empty())
    {
        client.sendMessage(":ircserv 461 " + client.getNickname()
                           + " TOPIC :Not enough parameters\r\n");
        return false;
    }
    server.topicCommand(client, params);
    return false;
}

bool handle_invite(Client &client, const std::vector<std::string> &args,
                   const std::string &params, Server &server)
{
    // En az iki parametre olmalı: <nick> <channel>
    if (args.size() < 2)
    {
        client.sendMessage(":ircserv 461 " + client.getNickname()
                           + " INVITE :Not enough parameters\r\n");
        return false;
    }
    server.inviteCommand(client, params);
    return false;
}

bool handle_kick(Client &client, const std::vector<std::string> &args, Server &server)
{
    // Format: KICK #channel nick [:reason]
    if (args.size() < 2)
    {
        client.sendMessage(":ircserv 461 " + client.getNickname()
                           + " KICK :Not enough parameters\r\n");
        return false;
    }
    const std::string &channelName = args[0];
    const std::string &targetNick  = args[1];
    std::string reason = args.size() >= 3 ? args[2] : "Kicked";
    server.handleKick(client, channelName, targetNick, reason);
    return false;
}

static void handle_channel_mode_query(Client &client, const std::string &target, Server &server)
{
    std::map<std::string, Channel> &channels = server.getChannels();
    std::map<std::string, Channel>::iterator it = channels.find(target);
    if (it == channels.end())
    {
        client.sendMessage(":ircserv 403 " + client.getNickname() + " " + target + " :No such channel\r\n");
        return;
    }

    std::string modes = "+";
    std::string modeArgs = "";
    if (it->second.isInviteOnly()) modes += "i";
    if (it->second.isTopicOpOnly()) modes += "t";
    if (!it->second.getKey().empty())
    {
        modes += "k";
        modeArgs += " " + it->second.getKey();
    }
    if (it->second.getLimit() > 0)
    {
        modes += "l";
        std::stringstream ss;
        ss << it->second.getLimit();
        modeArgs += " " + ss.str();
    }
    
    modes = (modes == "+") ? "" : " " + modes;
    client.sendMessage(":ircserv 324 " + client.getNickname() + " " + target + modes + modeArgs + "\r\n");
}

static void handle_user_mode_query(Client &client, const std::string &target)
{
    if (target == client.getNickname())
        client.sendMessage(":ircserv 221 " + client.getNickname() + " +i\r\n");
    else
        client.sendMessage(":ircserv 502 " + client.getNickname() + " :Cannot change mode for other users\r\n");
}

bool handle_mode(Client &client, const std::vector<std::string> &args, Server &server)
{
    if (args.empty())
    {
        client.sendMessage(":ircserv 461 " + client.getNickname() + " MODE :Not enough parameters\r\n");
        return false;
    }
    if (args.size() == 1)
    {
        if (args[0][0] == '#')
            handle_channel_mode_query(client, args[0], server);
        else
            handle_user_mode_query(client, args[0]);
        return false;
    }
    
    std::vector<std::string> modeParams(args.begin() + 2, args.end());
    server.handleMode(client, args[0], args[1], modeParams);
    return false;
}

// Kayıtlı client'tan gelen bir IRC satırını işler.
// Bağlantının kesilmesi gerekiyorsa true döner (örn. QUIT).
bool dispatch_command(Client &client, const std::string &line,
                      std::map<int, Client> &clients, Server &server) //zülal değiştirdim
{
    ParsedCommand parsed = parse_line(line);
    if (parsed.command.empty()) return false;

    std::string params;
    for (size_t i = 0; i < parsed.args.size(); ++i)
        params += (i > 0 ? " " : "") + parsed.args[i];

    const std::string &cmd = parsed.command;
    const std::vector<std::string> &args = parsed.args;

    if (cmd == "PING") return handle_ping(client, args, params);
    if (cmd == "QUIT") return handle_quit(client, params);
    if (cmd == "NICK") return handle_nick(client, args, clients);
    if (cmd == "JOIN") return handle_join(client, args, server);
    if (cmd == "PRIVMSG") { server.sendServerMessage(client, params); return false; }
    if (cmd == "PART") return handle_part(client, args, params, server);
    if (cmd == "TOPIC") return handle_topic(client, args, params, server);
    if (cmd == "INVITE") return handle_invite(client, args, params, server);
    if (cmd == "KICK") return handle_kick(client, args, server);
    if (cmd == "MODE") return handle_mode(client, args, server);

    client.sendMessage(":ircserv 421 " + client.getNickname() + " " + cmd + " :Unknown command\r\n");
    return false;
}

bool handle_client_message(Client &client, const std::string &line,
                           const std::string &serverPassword,
                           std::map<int, Client> &clients, Server &server) //zülal ekledi
{
    if (!client.isRegistered())
    {
        registration_state(client, line, serverPassword, clients);
        return false;
    }

    return dispatch_command(client, line, clients, server);
}