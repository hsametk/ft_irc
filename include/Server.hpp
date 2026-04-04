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
    void    receiveFromClient(int fd);
    void    removeClient(int fd);
    void            addPollFd(int fd);
    static void     signalHandler(int signum);
    Server(const Server &other);
    Server &operator=(const Server &other);

public:
    Server(int port, const std::string &password);
    ~Server();
    void    run();

};

#endif