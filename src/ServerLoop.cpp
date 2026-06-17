#include "../include/Server.hpp"
#include "../include/Auth.hpp"
#include <cstring>
#include <iostream>
#include <cstdlib>

// Public: Event Loop
void Server::run()
{
    int ready;
    while (_running)
    {
        // Update events for POLLOUT
        for (size_t i = 0; i < _pfds.size(); ++i)
        {
            if (_pfds[i].fd != _serverFd)
            {
                std::map<int, Client>::iterator it = _clients.find(_pfds[i].fd);
                if (it != _clients.end() && !it->second.getSendBuffer().empty())
                    _pfds[i].events = POLLIN | POLLOUT;
                else
                    _pfds[i].events = POLLIN;
            }
        }

        // poll() returns the number of file descriptors that are ready for I/O
        // -1 means wait indefinitely
        ready = poll(&_pfds[0], _pfds.size(), -1);
        if (ready == -1)
        {
            // if _running is false, it means the signal handler was called
            if (!_running)
                break;
            std::cerr << "Error: poll() failed\n";
            break;
        }

        // Snapshot current fds before iterating: receiveFromClient may call
        // removeClient which shrinks _pfds, invalidating indices.
        std::vector<int> activeFds;
        for (size_t i = 0; i < _pfds.size(); ++i)
            activeFds.push_back(_pfds[i].fd);

        // loop through all file descriptors
        for (size_t i = 0; i < activeFds.size(); i++)
        {
            // Find the pollfd for this fd (it may have been removed already)
            struct pollfd *pfd = NULL;
            for (size_t j = 0; j < _pfds.size(); ++j)
            {
                if (_pfds[j].fd == activeFds[i])
                {
                    pfd = &_pfds[j];
                    break;
                }
            }
            if (!pfd)
                continue;

            if (pfd->fd == _serverFd)
            {
                if (pfd->revents & POLLIN)
                    acceptClient();
                continue;
            }

            if (pfd->revents & POLLIN)
                receiveFromClient(pfd->fd);

            // Re-find pfd because receiveFromClient may have removed it
            pfd = NULL;
            for (size_t j = 0; j < _pfds.size(); ++j)
            {
                if (_pfds[j].fd == activeFds[i])
                {
                    pfd = &_pfds[j];
                    break;
                }
            }
            if (!pfd)
                continue;

            if (pfd->revents & POLLOUT)
                sendToClient(pfd->fd);
        }
    }
}

void Server::sendToClient(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    std::string& buffer = it->second.getSendBuffer();
    if (buffer.empty())
        return;

    ssize_t bytesSent = send(fd, buffer.c_str(), buffer.size(), 0);
    if (bytesSent < 0)
    {
        removeClient(fd);
        return;
    }
    
    it->second.eraseFromSendBuffer(bytesSent);
}

// Private: Accept New Client
void Server::acceptClient()
{
    int clientFd;
    // Accept new client
    clientFd = accept(_serverFd, NULL, NULL);
    if (clientFd == -1)
    {
        std::cerr << "Error: accept() failed\n";
        return;
    }
    // Set client socket to non-blocking
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
    {
        close(clientFd);
        return;
    }
    // Check if max clients reached
    if ((int)_pfds.size() >= MAX_CLIENTS)
    {
        std::cerr << "Max clients reached, rejecting fd " << clientFd << "\n";
        close(clientFd);
        return;
    }
    // Add client to poll set
    addPollFd(clientFd);
    // Add client to clients map
    //TODO: Client constructor'ı yazılmalı ve buraya eklenmeli burada yeni kullanıcı oluşturuluyor.
    _clients[clientFd] = Client(clientFd);
    std::cout << "New client connected: fd=" << clientFd << "\n";
}

void Server::receiveFromClient(int fd)
{
    char rawBuffer[512];
    ssize_t bytesRead;
    std::map<int, Client>::iterator it;

    std::memset(rawBuffer, 0, sizeof(rawBuffer));
    bytesRead = recv(fd, rawBuffer, sizeof(rawBuffer) - 1, 0); // null terminate için -1

    if (bytesRead > 0)
    {
        //TODO: Debug
        std::cout << "[DEBUG RAW " << fd << "] received " << bytesRead << " bytes: \"";
        for (ssize_t i = 0; i < bytesRead; ++i) {
            if (rawBuffer[i] == '\r') std::cout << "\\r";
            else if (rawBuffer[i] == '\n') std::cout << "\\n";
            else std::cout << rawBuffer[i];
        }
        std::cout << "\"" << std::endl;
    }

    if (bytesRead == 0)
    {
        std::cout << "Client disconnected: fd=" << fd << std::endl;
        removeClient(fd);
        return;
    }

    if (bytesRead < 0)
    {
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
        // Auth / Command routing
        if (handle_client_message(it->second, lines[i], _password, _clients, *this))
        {
            removeClient(fd);
            return;
        }
    }
}

// Client buffer içinden tamamlanmış satırları çıkarır.
// \r\n ve \n sonlandırıcılarını destekler.
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