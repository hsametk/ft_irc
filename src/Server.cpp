#include "../include/Server.hpp"

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
    // Close all client sockets
    for (size_t i = 0; i < _pfds.size(); i++)
        close(_pfds[i].fd);
    std::cout << "Server shut down.\n";
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
// Private: Read From Client
void Server::receiveFromClient(int fd)
{
    // Receive data from client
    char    buf[512];
    ssize_t bytes;
    
    bytes = recv(fd, buf, sizeof(buf) - 1, 0);
    // Check if client disconnected
    if (bytes <= 0)
    {
        // Print error message
        if (bytes == 0)
            std::cout << "Client fd=" << fd << " disconnected.\n";
        else
            // Print error message
            std::cerr << "Error: recv() failed on fd=" << fd << "\n";
        // Remove client
        removeClient(fd);
        return;
    }
    buf[bytes] = '\0';
    // Append data to client buffer
    _clients[fd].appendToBuffer(buf);
    // Extract and log complete commands (split by \r\n)
    std::string &buffer = _clients[fd].getBuffer();
    size_t pos;
    while ((pos = buffer.find("\r\n")) != std::string::npos)
    {
        std::string line = buffer.substr(0, pos);
        buffer.erase(0, pos + 2);
        if (!line.empty())
            std::cout << "[fd=" << fd << "] cmd: " << line << "\n";
    }
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
    // Create pollfd
    struct pollfd pfd;
    pfd.fd      = fd;
    pfd.events  = POLLIN;
    pfd.revents = 0;
    // Add fd to poll set
    _pfds.push_back(pfd);
}