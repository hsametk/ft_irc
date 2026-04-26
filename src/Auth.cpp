
#include "../include/Auth.hpp"
#include <iostream>
#include <map>

// Params string'inden sondaki \r veya boşlukları temizler
static std::string trim(const std::string &s)
{
    size_t end = s.size();
    while (end > 0 && (s[end - 1] == '\r' || s[end - 1] == ' '))
        --end;
    return s.substr(0, end);
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
    client.sendMessage(":ft_irc 372 " + nick + " :- Available commands: JOIN, PRIVMSG, NICK, USER\r\n");
    client.sendMessage(":ft_irc 372 " + nick + " :- Example: /join #42\r\n");
    client.sendMessage(":ft_irc 376 " + nick + " :End of MOTDmacommand\r\n");
}

void registration_state(Client &client, const std::string &line,
                        const std::string &serverPassword,
                        const std::map<int, Client> &clients)
{
    std::string command;
    std::string params;

    size_t spacePos = line.find(' ');
    if (spacePos != std::string::npos)
    {
        command = trim(line.substr(0, spacePos));
        params  = trim(line.substr(spacePos + 1));
    }
    else
    {
        command = trim(line);
        params  = "";
    }

    // CAP: irssi capability müzakeresi
    if (command == "CAP")
    {
        if (params.substr(0, 2) == "LS")
            client.sendMessage(":ircserv CAP * LS :\r\n");
        return;
    }

    if (command == "PASS")
    {
        if (params == serverPassword)
        {
            client.setPassOk(true);
            std::cout << "PASS accepted for fd=" << client.getFd() << std::endl;
        }
        else
        {
            std::cerr << "Wrong password from fd=" << client.getFd() << std::endl;
            // TODO: 464 ERR_PASSWDMISMATCH gönder
        }
    }
    else if (command == "NICK")
    {
        if (params.empty())
        {
            // 431 ERR_NONICKNAMEGIVEN
            client.sendMessage(":ircserv 431 * :No nickname given\r\n");
            return;
        }
        if (isNickInUse(clients, params, client.getFd()))
        {
            // 433 ERR_NICKNAMEINUSE
            client.sendMessage(":ircserv 433 * " + params + " :Nickname is already in use\r\n");
            std::cerr << "Nick '" << params << "' already in use, rejected fd="
                      << client.getFd() << std::endl;
            return;
        }
        client.setNickname(params);
        client.setNickSet(true);
        std::cout << "NICK set: " << params << " for fd=" << client.getFd() << std::endl;
    }
    else if (command == "USER")
    {
        client.setUserSet(true);
        std::cout << "USER set: " << params << std::endl;
    }

    // Üç koşul da sağlandı mı? -> kayıt tamamlandı
    if (client.isPassOk() && client.isNickSet() && client.isUserSet())
    {
        client.setRegistered(true);
        std::cout << "Client fd=" << client.getFd() << " is now registered!" << std::endl;
        send_welcome(client);
    }
}

// Kayıtlı client'tan gelen bir IRC satırını isle.
// Bagintinin kesilmesi gerekiyorsa true doner (ornegin QUIT).
bool dispatch_command(Client &client, const std::string &line,
                      std::map<int, Client> &clients, Server &server)
{
    (void)clients; // Henuz kullanilanmayan parametreleri susturur

    std::string command;
    std::string params;

    size_t spacePos = line.find(' ');
    if (spacePos != std::string::npos)
    {
        command = line.substr(0, spacePos);
        params  = line.substr(spacePos + 1);
        // Sondaki bosluk/CR temizle
        size_t end = params.size();
        while (end > 0 && (params[end - 1] == ' ' || params[end - 1] == '\r'))
            --end;
        params = params.substr(0, end);
    }
    else
    {
        command = line;
        // Sondaki CR temizle
        size_t end = command.size();
        while (end > 0 && (command[end - 1] == ' ' || command[end - 1] == '\r'))
            --end;
        command = command.substr(0, end);
        params  = "";
    }

    // --- PING ---
    // Server'in hayatta oldugunu dogrulamak icin client periyodik PING gonderir.
    // RFC 2812: cevap PONG :token olmali.
    if (command == "PING")
    {
        client.sendMessage(":ircserv PONG ircserv :" + params + "\r\n");
        return false;
    }

    // --- QUIT ---
    // Client baglantisini temiz kapatmak istiyor.
    if (command == "QUIT")
    {
        std::string reason = params.empty() ? "Client quit" : params;
        // ':' ile baslayan trailing parametreyi soy.
        if (!reason.empty() && reason[0] == ':')
            reason = reason.substr(1);
        client.sendMessage(":ircserv ERROR :Closing connection (" + reason + ")\r\n");
        std::cout << "QUIT from fd=" << client.getFd()
                  << " reason: " << reason << std::endl;
        return true; // Caller removeClient() cagirsin
    }

    // --- TODO: PART, PRIVMSG, NOTICE, MODE, TOPIC, KICK, INVITE ---
    if (command == "JOIN")
    {
        server.joinChannel(client, params);
        return false;
    }

    // --- PART ---
    // Client bir kanaldan ayrılmak istiyor.
    if (command == "PART")
    {
        server.partChannel(client, params);
        return false;
    }

    // Bilinmeyen komut -> 421 ERR_UNKNOWNCOMMAND
    client.sendMessage(":ircserv 421 " + client.getNickname()
                       + " " + command + " :Unknown command\r\n");
    return false;
}

bool handle_client_message(Client &client, const std::string &line,
                           const std::string &serverPassword,
                           std::map<int, Client> &clients, Server &server)
{
    if (!client.isRegistered())
    {
        registration_state(client, line, serverPassword, clients);
        return false; // Kayıt sırasında (henüz) hemen bağlantı kopartma yapmıyoruz
    }
    
    // Kayıtlıysa normal komut işlemeye gönder
    return dispatch_command(client, line, clients, server);
}