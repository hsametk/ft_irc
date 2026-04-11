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

        // Bir client'tan veri okur, buffer'a ekler ve tam satırları ayıklar.
        void receiveFromClient(int fd);

        // Client'ı poll listesinden ve map'ten temizler.
        void removeClient(int fd);

        // Client buffer içinden tamamlanmış satırları çıkarır.
        // \r\n ve \n sonlandırıcılarını destekler.
        std::vector<std::string> extractLines(Client& client);
};

#endif