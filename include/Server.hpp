#ifndef SERVER_HPP
#define SERVER_HPP


#include <iostream>
#include <vector> //-> for vector
#include <sys/socket.h> //-> for socket()
#include <sys/types.h> //-> for socket()
#include <netinet/in.h> //-> for sockaddr_in
#include <fcntl.h> //-> for fcntl()
#include <unistd.h> //-> for close()
#include <arpa/inet.h> //-> for inet_ntoa()
#include <poll.h> //-> for poll()
#include <csignal> //-> for signal()
#include <map>
#include <string>
#include "Client.hpp"
class Server
{
private:
    int                         _serverFd;
    int                         _port;
    std::string                 _password;
    std::vector<struct pollfd>  _pfds;
    std::map<int, Client>       _clients;
    int _port;
    std::string _password;
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
};

Server::Server(/* args */)
{
}

Server::~Server()
{
}

#endif