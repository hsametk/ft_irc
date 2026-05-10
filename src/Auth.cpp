#include "../include/Auth.hpp"
#include <iostream>
#include <map>

std::string normalize_spaces(const std::string &line) //zülal
{
    std::string result;
    bool inSpace = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')
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

    if (!result.empty() && result[0] == ' ')
        result.erase(0, 1);

    if (!result.empty() && result[result.size() - 1] == ' ')
        result.erase(result.size() - 1);

    return result;
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
    size_t start = 0;

    while (start < rest.size())
    {
        size_t space = rest.find(' ', start);

        if (space == std::string::npos)
        {
            parsed.args.push_back(rest.substr(start));
            break;
        }

        parsed.args.push_back(rest.substr(start, space - start));
        start = space + 1;
    }

    return parsed;
}


// clients map'inde istenen nick kullanımda mı? (kendisi hariç)
static bool isNickInUse(const std::map<int, Client> &clients,
                        const std::string &nick, int selfFd)
{
    for (std::map<int, Client>::const_iterator it = clients.begin();
         it != clients.end(); ++it)
    {
        if (it->first != selfFd && it->second.getNickname() == nick)
            return true;
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
        client.sendMessage(":ircserv 421 * " + command + " :Unknown command\r\n");
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
    if (command == "JOIN")
    {
        if (args.empty())
        {
            client.sendMessage(":ircserv 461 " + client.getNickname() + " JOIN :Not enough parameters\r\n");
            return false;
        }

        server.joinChannel(client, params);
        return false;
    }

    // --- PART ---
    if (command == "PART")
    {
        if (args.empty())
        {
            client.sendMessage(":ircserv 461 " + client.getNickname()
                               + " PART :Not enough parameters\r\n");
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