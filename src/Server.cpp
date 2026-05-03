#include "../include/Server.hpp"
// #include "../include/Auth.hpp"
#include <cerrno>
#include <cstring>

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

// Signal Handler
void Server::signalHandler(int signum)
{
    (void)signum;
    std::cout << "\nSignal received, shutting down...\n";
    _running = false;
}

// Send error message to client
void Server::sendError(Client& client, int code, const std::string& msg)
{
    std::stringstream ss;
    ss << code;
    std::string errorMsg = ":ircserv " + ss.str() + " " + client.getNickname() + " " + msg + "\r\n";
    send(client.getFd(), errorMsg.c_str(), errorMsg.size(), 0);
}
