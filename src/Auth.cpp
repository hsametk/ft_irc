
#include "../include/Server.hpp"
#include "../include/Auth.hpp"
#include <iostream>
#include <map>
#include <cctype>

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


// clients map'inde istenen nick kullanımda mı? (kendisi hariç)
// Params string'inden sondaki \r veya boşlukları temizler
// static std::string trim(const std::string &s)
// {
//     size_t end = s.size();
//     while (end > 0 && (s[end - 1] == '\r' || s[end - 1] == ' '))
//         --end;
//     return s.substr(0, end);
// }

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

void registration_state(Client &client, const std::string &line,
                        const std::string &serverPassword,
                        const std::map<int, Client> &clients) //zülal değiştirdim
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
    else if (command == "NICK")
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
    else if (command == "USER")
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

// Kayıtlı client'tan gelen bir IRC satırını işler.
// Bağlantının kesilmesi gerekiyorsa true döner (örn. QUIT).
bool dispatch_command(Client &client, const std::string &line,
                      std::map<int, Client> &clients, Server &server) //zülal değiştirdim
{
    ParsedCommand parsed = parse_line(line);

    if (parsed.command.empty())
        return false;

    std::string command = parsed.command;
    std::vector<std::string> args = parsed.args;

    std::string params;
    for (size_t i = 0; i < args.size(); ++i)
    {
        if (i > 0)
            params += " ";
        params += args[i];
    }

    // --- PING ---
    if (command == "PING")
    {
        if (args.empty())
            client.sendMessage(":ircserv 409 " + client.getNickname() + " :No origin specified\r\n");
        else
            client.sendMessage(":ircserv PONG ircserv :" + params + "\r\n");
        return false;
    }

    // --- QUIT ---
    if (command == "QUIT")
    {
        std::string reason = params.empty() ? "Client quit" : params;

        if (!reason.empty() && reason[0] == ':')
            reason = reason.substr(1);

        client.sendMessage(":ircserv ERROR :Closing connection (" + reason + ")\r\n");
        std::cout << "QUIT from fd=" << client.getFd()
                  << " reason: " << reason << std::endl;
        return true;
    }

    // --- NICK ---
    if (command == "NICK")
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

    // --- JOIN ---
    // Çoklu kanal desteği: "JOIN #a,#b key1,key2"
    if (command == "JOIN")
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

    if (command == "PRIVMSG") {
        server.sendServerMessage(client, params);
        return false;
    }

    // --- PART ---
    if (command == "PART")
    {
        if (args.empty())
        {
            client.sendMessage(":ircserv 461 " + client.getNickname() + " PART :Not enough parameters\r\n");
            return false;
        }
        server.partChannel(client, params);
        return false;
    }

    // --- TOPIC ---
    if (command == "TOPIC")
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

    // --- INVITE ---
    if (command == "INVITE")
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

    // Bilinmeyen komut -> 421 ERR_UNKNOWNCOMMAND
    client.sendMessage(":ircserv 421 " + client.getNickname()
                       + " " + command + " :Unknown command\r\n");
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