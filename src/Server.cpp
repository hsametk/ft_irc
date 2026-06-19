#include "../include/Server.hpp"
#include <sstream>
#include <set>

// ---------------------------------------------------------------------------
// ServerClient — sunucunun yaşam döngüsü ve client'a yönelik yardımcıları:
// kurulum/kapanış, sinyal yönetimi, hata mesajı gönderimi ve NICK duyurusu.
// ---------------------------------------------------------------------------

bool Server::_running = true;

// C++98 uyumlu int → string dönüşümü (std::to_string yok).
static std::string intToStr(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

// --- Constructor / Destructor ---
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
    for (std::map<int, Client>::iterator it = _clients.begin();
         it != _clients.end(); ++it)
        close(it->first);
    if (_serverFd != -1)
        close(_serverFd);
    std::cout << "Server shut down cleanly.\n";
}

// --- Signal handler ---
// SIGINT / SIGTERM geldiğinde event loop'u durdurur.
void Server::signalHandler(int signum)
{
    (void)signum;
    std::cout << "\nSignal received, shutting down...\n";
    _running = false;
}

// --- Hata mesajı gönderimi ---
// Standart IRC numeric formatında hata yollar: ":ircserv <code> <nick> <msg>"
void Server::sendError(Client &client, int code, const std::string &msg)
{
    std::string errorMsg = ":ircserv " + intToStr(code) + " "
                         + client.getNickname() + " " + msg + "\r\n";
    send(client.getFd(), errorMsg.c_str(), errorMsg.size(), 0);
}

// --- NICK değişikliği duyurusu ---
// Kullanıcının bulunduğu tüm kanallardaki üyelere NICK değişimini bildirir.
// Aynı kullanıcıya birden fazla ortak kanaldan tekrar gönderilmemesi için
// fd bazlı deduplicate uygulanır.
void Server::broadcastNickChange(Client &client, const std::string &oldNick)
{
    std::string nickMsg = ":" + oldNick + "!" + client.getUsername()
                        + "@localhost NICK " + client.getNickname() + "\r\n";

    std::set<int> notified;
    notified.insert(client.getFd()); // Kendisine zaten gönderildi.

    for (std::map<std::string, Channel>::iterator ch = _channels.begin();
         ch != _channels.end(); ++ch)
    {
        if (!ch->second.hasMember(client.getFd()))
            continue;

        const std::map<int, Client*> &members = ch->second.getMembers();
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