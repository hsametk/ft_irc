#include "../include/Server.hpp"
#include "../include/Auth.hpp"
#include <cstring>
#include <iostream>

// ---------------------------------------------------------------------------
// ServerLoop — event loop ve I/O: poll döngüsü, yeni bağlantı kabulü,
// client'tan veri okuma ve satır ayıklama.
// ---------------------------------------------------------------------------

// --- Event loop ---
// Tek poll() çağrısıyla tüm fd'leri izler; hazır olanları işler.
void Server::run()
{
    while (_running)
    {
        // poll(): I/O için hazır fd sayısını döner, -1 = süresiz bekle.
        int ready = poll(&_pfds[0], _pfds.size(), -1);
        if (ready == -1)
        {
            // _running false ise sinyal handler tetiklenmiştir.
            if (!_running)
                break;
            std::cerr << "Error: poll() failed\n";
            break;
        }

        // Silme işlemlerinden etkilenmemek için mevcut boyut üzerinden gez.
        size_t currentSize = _pfds.size();
        for (size_t i = 0; i < currentSize; i++)
        {
            if (!(_pfds[i].revents & POLLIN))
                continue;
            // Server fd ise yeni client, değilse veri okuması.
            if (_pfds[i].fd == _serverFd)
                acceptClient();
            else
                receiveFromClient(_pfds[i].fd);
        }
    }
}

// --- Yeni bağlantı kabulü ---
void Server::acceptClient()
{
    int clientFd = accept(_serverFd, NULL, NULL);
    if (clientFd == -1)
    {
        std::cerr << "Error: accept() failed\n";
        return;
    }

    // Client soketini non-blocking yap.
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
    {
        close(clientFd);
        return;
    }

    // Maksimum client sayısına ulaşıldıysa reddet.
    if ((int)_pfds.size() >= MAX_CLIENTS)
    {
        std::cerr << "Max clients reached, rejecting fd " << clientFd << "\n";
        close(clientFd);
        return;
    }

    addPollFd(clientFd);
    _clients[clientFd] = Client(clientFd);
    std::cout << "New client connected: fd=" << clientFd << "\n";
}

// --- Client'tan veri okuma ---
// poll() POLLIN dediği için recv() çağrılır. Dönüş <= 0 ise bağlantı biter;
// aksi halde veri buffer'a eklenir ve satırlar işlenir.
void Server::receiveFromClient(int fd)
{
    char rawBuffer[512];
    std::memset(rawBuffer, 0, sizeof(rawBuffer));

    ssize_t bytesRead = recv(fd, rawBuffer, sizeof(rawBuffer) - 1, 0);

    // 0 = karşı taraf kapattı, <0 = hata (POLLIN sonrası EAGAIN olmaz).
    if (bytesRead <= 0)
    {
        if (bytesRead == 0)
            std::cout << "Client disconnected: fd=" << fd << std::endl;
        else
            std::cerr << "recv() error on fd " << fd << std::endl;
        removeClient(fd);
        return;
    }

    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    // Partial veri olabileceği için doğrudan parse etmeyip buffer'a ekliyoruz.
    it->second.appendToBuffer(std::string(rawBuffer, bytesRead));

    // Buffer aşırı büyürse bağlantıyı kapat.
    if (it->second.getBuffer().size() > MAX_BUFFER_SIZE)
    {
        std::cerr << "Buffer overflow, dropping client fd=" << fd << std::endl;
        removeClient(fd);
        return;
    }

    processLines(it->second, fd);
}

// --- Buffer'daki tam satırları işle ---
// Tamamlanmış her satırı router'a verir; bağlantı kapanması gerekirse durur.
void Server::processLines(Client &client, int fd)
{
    std::vector<std::string> lines = extractLines(client);

    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].empty())
            continue;

        if (handle_client_message(client, lines[i], _password, _clients, *this))
        {
            removeClient(fd);
            return;
        }
    }
}

// --- Satır ayıklama ---
// Buffer içinden tamamlanmış satırları çıkarır. \r\n ve \n destekler.
std::vector<std::string> Server::extractLines(Client &client)
{
    std::vector<std::string> lines;
    std::string &buffer = client.getBuffer();

    while (true)
    {
        size_t crlfPos = buffer.find("\r\n");
        size_t lfPos   = buffer.find('\n');

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

        // Satır başında artık \r kalmışsa temizle.
        if (!line.empty() && line[0] == '\r')
            line.erase(0, 1);

        lines.push_back(line);
        buffer.erase(0, cutPos + delimiterLength);
    }

    return lines;
}
