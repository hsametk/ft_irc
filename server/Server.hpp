#include <string>
#ifndef SERVER.HPP
#define SERVER.HPP

class Server
{
private:
    int server_socke_fd;
    int pollfd[1024];
    std::string client_list[1024];
    std::string channel_list[1024];
public:
    Server(/* args */);
    ~Server();
};

Server::Server(/* args */)
{
}

Server::~Server()
{
}

#endif