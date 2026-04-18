#include "../include/Server.hpp"
#include "../include/Auth.hpp"
#include <cerrno>
#include <cstring>

#define MAX_BUFFER_SIZE 8192

bool Server::_running = true;

// Constructor / Destructor
Server::Server(int port, const std::string &password)
    : _serverFd(-1), _port(port), _password(password)
{
    signal(SIGINT,  Server::signalHandler);
    signal(SIGTERM, Server::signalHandler);
    try
    {
        initServer();
    }
    catch (...)
    {
        if (_serverFd != -1)
            close(_serverFd);
        throw;
    }
}

Server::~Server()
{
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        close(it->first);
    if (_serverFd != -1)
        close(_serverFd);
    std::cout << "Server shut down cleanly.\n";
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
        // Auth / Command routing
        if (handle_client_message(it->second, lines[i], _password, _clients, *this))
        {
            removeClient(fd);
            return;
        }
    }
}

void Server::removeClient(int fd)
{
    // Poll setinden çıkar.
    for (size_t i = 0; i < _pfds.size(); ++i)
    {
        if (_pfds[i].fd == fd)
        {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }
    // Soketi kapat ve clients map'inden sil.
    close(fd);
    _clients.erase(fd);
    std::cout << "Client removed: fd=" << fd << std::endl;
}
// Signal Handler
void Server::signalHandler(int signum)
{
    (void)signum;
    std::cout << "\nSignal received, shutting down...\n";
    _running = false;
}
// Private: Initialise Socket
void Server::initServer()
{
    // 1. Create TCP socket
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd == -1)
        throw std::runtime_error("Error: socket() failed");
    // 2. Non-blocking
    if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("Error: fcntl() failed");
    // 3. Allow address reuse
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        throw std::runtime_error("Error: setsockopt() failed");
    // 4. Bind to port
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverFd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("Error: bind() failed");

    // 5. Start listening
    if (listen(_serverFd, 5) == -1)
        throw std::runtime_error("Error: listen() failed");

    // 6. Add server fd to poll set
    addPollFd(_serverFd);

    std::cout << "Server listening on port " << _port << "\n";
}
// Public: Event Loop
void Server::run()
{
    int ready;
    size_t currentSize;
    while (_running)
    {
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

        // Iterate over a *copy* of current size so we don't trip on removals
        currentSize = _pfds.size();
        // loop through all file descriptors
        for (size_t i = 0; i < currentSize; i++)
        {
            // if revents is not POLLIN, continue
            if (!(_pfds[i].revents & POLLIN))
                continue;
            // if revents is POLLIN, it means the file descriptor is ready for I/O
            // if the file descriptor is the server fd, accept the new client
            if (_pfds[i].fd == _serverFd)
                acceptClient();
            // else, receive data from the client
            else
                receiveFromClient(_pfds[i].fd);
        }
    }
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
// Private: Add fd to poll set
void Server::addPollFd(int fd)
{
    // Create pollfd
    struct pollfd pfd;
    pfd.fd      = fd;
    pfd.events  = POLLIN;
    pfd.revents = 0;
    // Add fd to poll set
    _pfds.push_back(pfd);
}

// --- JOIN ---
// client'\u0131 params'daki kanal(lar)a kat\u0131\u015ft\u0131r\u0131r.
// params format\u0131: "#kanal1,#kanal2" veya "#kanal \u015fifre"
void Server::joinChannel(Client &client, const std::string &params)
{
    // TODO: JOIN implementasyonu buraya gelecek
    (void)client;
    (void)params;
}