#include "../include/Server.hpp"
#include <cerrno>
#include <cstring>

Server::Server() : _serverFd(-1), _port(0), _password("")
{
}

Server::Server(int port, std::string password) : _serverFd(-1), _port(port), _password(password)
{
}

Server::Server(const Server& other)
{
    *this = other;
}

Server& Server::operator=(const Server& other)
{
    if (this != &other)
    {
        _serverFd = other._serverFd;
        _port = other._port;
        _password = other._password;
        _pfds = other._pfds;
        _clients = other._clients;
    }
    return *this;
}

Server::~Server()
{
    if (_serverFd != -1)
        close(_serverFd);
}

void Server::initServer()
{
}

void Server::run()
{
}

void Server::acceptClient()
{
}

std::vector<std::string> Server::extractLines(Client& client)
{
    std::vector<std::string> lines;
    std::string& buffer = client.getBuffer();
    size_t pos;

    while ((pos = buffer.find("\r\n")) != std::string::npos)
    {
        lines.push_back(buffer.substr(0, pos));
        buffer.erase(0, pos + 2);
    }
    return lines;
}

void Server::receiveFromClient(int fd)
{
    char rawBuffer[512];
    ssize_t bytesRead;
    std::map<int, Client>::iterator it;

    std::memset(rawBuffer, 0, sizeof(rawBuffer));
    bytesRead = recv(fd, rawBuffer, sizeof(rawBuffer), 0);

    if (bytesRead == 0)
    {
        std::cout << "Client disconnected: fd=" << fd << std::endl;
        removeClient(fd);
        return;
    }

    if (bytesRead < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        std::cerr << "recv() error on fd " << fd << std::endl;
        removeClient(fd);
        return;
    }

    it = _clients.find(fd);
    if (it == _clients.end())
        return;

    it->second.appendToBuffer(std::string(rawBuffer, bytesRead));

    if (it->second.getBuffer().size() > 8192)
    {
        std::cerr << "Client buffer too large: fd=" << fd << std::endl;
        removeClient(fd);
        return;
    }

    std::vector<std::string> lines = extractLines(it->second);
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].empty())
            continue;

        std::cout << "fd[" << fd << "] line: [" << lines[i] << "]" << std::endl;
    }
}

void Server::removeClient(int fd)
{
    for (size_t i = 0; i < _pfds.size(); ++i)
    {
        if (_pfds[i].fd == fd)
        {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }

    close(fd);
    _clients.erase(fd);
}