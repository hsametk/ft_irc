#include "../include/Server.hpp"

// Constructor / Destructor
Server::Server(int port, const std::string &password)
    : _serverFd(-1), _port(port), _password(password)
{
    initServer();
}

Server::~Server()
{
    for (size_t i = 0; i < _pfds.size(); i++)
        close(_pfds[i].fd);
    std::cout << "Server shut down.\n";
}
// Private: Initialise Socket
void Server::initServer()
{
    // 1. Create TCP socket
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd == -1)
        throw std::runtime_error("Error: socket() failed");

    // 2. Allow address reuse (avoids "address already in use" on restart)
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        throw std::runtime_error("Error: setsockopt() failed");

    // 3. Bind to port
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverFd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("Error: bind() failed");

    // 4. Start listening
    if (listen(_serverFd, 5) == -1)
        throw std::runtime_error("Error: listen() failed");

    // 5. Add server fd to poll set
    addPollFd(_serverFd);

    std::cout << "Server listening on port " << _port << "\n";
}
// Public: Event Loop
void Server::run()
{
    while (true)
    {
        int ready = poll(&_pfds[0], _pfds.size(), -1);
        if (ready == -1)
        {
            std::cerr << "Error: poll() failed\n";
            break;
        }

        // Iterate over a *copy* of current size so we don't trip on removals
        size_t currentSize = _pfds.size();
        for (size_t i = 0; i < currentSize; i++)
        {
            if (!(_pfds[i].revents & POLLIN))
                continue;

            if (_pfds[i].fd == _serverFd)
                acceptClient();
            else
                receiveFromClient(_pfds[i].fd);
        }
    }
}
// Private: Accept New Client
void Server::acceptClient()
{
    int clientFd = accept(_serverFd, NULL, NULL);
    if (clientFd == -1)
    {
        std::cerr << "Error: accept() failed\n";
        return;
    }

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
// Private: Read From Client
void Server::receiveFromClient(int fd)
{
    char    buf[512];
    ssize_t bytes = recv(fd, buf, sizeof(buf) - 1, 0);

    if (bytes <= 0)
    {
        if (bytes == 0)
            std::cout << "Client fd=" << fd << " disconnected.\n";
        else
            std::cerr << "Error: recv() failed on fd=" << fd << "\n";
        removeClient(fd);
        return;
    }
    buf[bytes] = '\0';
    _clients[fd].appendToBuffer(buf);
    // TODO: parse IRC messages from buffer (look for \r\n)
    std::cout << "[fd=" << fd << "] raw: " << buf;
}
// Private: Remove Client
void Server::removeClient(int fd)
{
    close(fd);
    _clients.erase(fd);

    // Remove from _pfds vector
    for (size_t i = 0; i < _pfds.size(); i++)
    {
        if (_pfds[i].fd == fd)
        {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }
}
// Private: Add fd to poll set
void Server::addPollFd(int fd)
{
    struct pollfd pfd;
    pfd.fd      = fd;
    pfd.events  = POLLIN;
    pfd.revents = 0;
    _pfds.push_back(pfd);
}