#include "../include/Server.hpp"
#include <cerrno>
#include <cstring>

#define MAX_BUFFER_SIZE 8192

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
    // Bu branch'in konusu değil.
}

void Server::run()
{
    // Bu branch'in konusu değil.
}

void Server::acceptClient()
{
    // Bu branch'in konusu değil.
}

std::vector<std::string> Server::extractLines(Client& client)
{
    std::vector<std::string> lines;
    std::string& buffer = client.getBuffer();

    while (true)
    {
        // Önce IRC standardındaki \r\n sonlandırıcısını arıyoruz.
        size_t crlfPos = buffer.find("\r\n");

        // Bazı testlerde veya basit client'larda sadece \n gelebilir.
        size_t lfPos = buffer.find('\n');

        if (crlfPos == std::string::npos && lfPos == std::string::npos)
            break;

        size_t cutPos;
        size_t delimiterLength;

        if (crlfPos != std::string::npos && (lfPos == std::string::npos || crlfPos < lfPos))
        {
            cutPos = crlfPos;
            delimiterLength = 2;
        }
        else
        {
            cutPos = lfPos;
            delimiterLength = 1;
        }

        std::string line = buffer.substr(0, cutPos);

        // Satır başında gereksiz \r kalmışsa temizle.
        if (!line.empty() && line[0] == '\r')
            line.erase(0, 1);

        lines.push_back(line);

        // İşlenen kısmı buffer'dan sil.
        buffer.erase(0, cutPos + delimiterLength);
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

    // Client bu sırada silinmiş olabilir.
    if (it == _clients.end())
        return;

    // Gelen veriyi doğrudan parse etmiyoruz.
    // Önce ilgili client'ın buffer'ına ekliyoruz.
    it->second.appendToBuffer(std::string(rawBuffer, bytesRead));

    // Buffer aşırı büyürse bağlantıyı kapat.
    if (it->second.getBuffer().size() > MAX_BUFFER_SIZE)
    {
        std::cerr << "Buffer overflow, dropping client fd=" << fd << std::endl;
        removeClient(fd);
        return;
    }

    std::vector<std::string> lines = extractLines(it->second);

    for (size_t i = 0; i < lines.size(); ++i)
    {
        // Boş satırları atla.
        if (lines[i].empty())
            continue;

        std::cout << "[CLIENT " << fd << "] -> " << lines[i] << std::endl;

        // Burada daha sonra parser / command handler çağrılabilir.
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