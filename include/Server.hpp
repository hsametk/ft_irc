#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include "Client.hpp"
#include "Channel.hpp"

#define MAX_CLIENTS 1024
#define MAX_BUFFER_SIZE 8192

// IRC Error codes
#define ERR_NOSUCHCHANNEL    403
#define ERR_USERONCHANNEL    443
#define ERR_BADCHANNELKEY    475
#define ERR_INVITEONLYCHAN   473
#define ERR_CHANNELISFULL    471

class Server
{
private:
    int                         _serverFd;
    int                         _port;
    std::string                 _password;
    std::vector<struct pollfd>  _pfds;
    std::map<int, Client>       _clients;
    static bool                 _running;
    void    initServer();
    void    acceptClient();
    void            addPollFd(int fd);
    static void     signalHandler(int signum);
    Server(const Server &other);
    Server &operator=(const Server &other);
    // Bir client'tan veri okur, buffer'a ekler ve tam satırları ayıklar.
    void receiveFromClient(int fd);
    // Client'ı poll listesinden ve map'ten temizler.
    void removeClient(int fd);
    // Client buffer içinden tamamlanmış satırları çıkarır.
    // \r\n ve \n sonlandırıcılarını destekler.
    std::vector<std::string> extractLines(Client& client);
    std::map<std::string, Channel> _channels;

public:
    Server(int port, const std::string &password);
    ~Server();
    void    run();

    // --- Kanal Komutları ---
    // client'ı params'da belirtilen kanala(lara) katıştırır.
    // params: "#kanal1,#kanal2 şifre1,şifre2" formatını destekler.
    void joinChannel(Client& client, const std::string& params);
    // client'ı params'da belirtilen kanaldan çıkarır.
    // params: "#kanal :ayrılma mesajı" formatını destekler.
    void partChannel(Client& client, const std::string& params);
    // Client'a hata mesajı gönderir.
    void sendError(Client& client, int code, const std::string& msg);
};

#endif