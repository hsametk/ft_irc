#include "../include/Server.hpp"
// #include "../include/Auth.hpp"


bool Server::_running = true;

// C++98 uyumlu int → string dönüşümü
static std::string intToStr(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

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
    std::string errorMsg = ":ircserv " + intToStr(code) + " " + client.getNickname() + " " + msg + "\r\n";
    client.sendMessage(errorMsg);
}

// NICK değişikliğini kullanıcının bulunduğu tüm kanallara duyurur.
// Aynı kullanıcıya birden fazla mesaj gitmemesi için fd bazlı deduplicate yapar.
void Server::broadcastNickChange(Client& client, const std::string& oldNick)
{
    std::string nickMsg = ":" + oldNick + "!" + client.getUsername()
                        + "@localhost NICK " + client.getNickname() + "\r\n";

    // Birden fazla ortak kanaldaki kullanıcıya yalnızca bir kez gönder.
    std::set<int> notified;
    notified.insert(client.getFd()); // Kendisine zaten gönderildi.

    for (std::map<std::string, Channel>::iterator ch = _channels.begin();
         ch != _channels.end(); ++ch)
    {
        if (!ch->second.hasMember(client.getFd()))
            continue;
        const std::map<int, Client*>& members = ch->second.getMembers();
        for (std::map<int, Client*>::const_iterator m = members.begin();
             m != members.end(); ++m)
        {
            if (notified.find(m->first) == notified.end())
            {
                m->second->sendMessage(nickMsg);
                notified.insert(m->first);
            }
        }
    }
}

