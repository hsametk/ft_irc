#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <csignal>
#include "Client.hpp"

class Server
{
    private:
        int                         _serverFd;
        int                         _port;
        std::string                 _password;
        std::vector<struct pollfd>  _pfds;
        std::map<int, Client>       _clients;

    public:
        Server();
        Server(int port, std::string password);
        Server(const Server& other);
        Server& operator=(const Server& other);
        ~Server();

        void initServer();
        void run();
        void acceptClient();
        void receiveFromClient(int fd);
        void removeClient(int fd);

        std::vector<std::string> extractLines(Client& client);
};

#endif