#include "../include/Server.hpp"

void Server::initServer()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd == -1)
        throw std::runtime_error("Error: socket() failed");
    if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("Error: fcntl() failed");
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        throw std::runtime_error("Error: setsockopt() failed");
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverFd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("Error: bind() failed");
    if (listen(_serverFd, 5) == -1)
        throw std::runtime_error("Error: listen() failed");
    addPollFd(_serverFd);
    std::cout << "Server listening on port " << _port << "\n";
}

void Server::addPollFd(int fd)
{
    struct pollfd pfd;
    pfd.fd      = fd;
    pfd.events  = POLLIN;
    pfd.revents = 0;
    // Add fd to poll set
    _pfds.push_back(pfd);
}