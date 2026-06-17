#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include "Client.hpp"
#include "Channel.hpp"

#define MAX_CLIENTS 1024
#define MAX_BUFFER_SIZE 8192


#define ERR_NOSUCHNICK        401
#define ERR_USERONCHANNEL     443
#define ERR_NOTONCHANNEL      442
#define ERR_NEEDMOREPARAMS    461
#define ERR_CHANOPRIVSNEEDED  482
#define ERR_BADCHANNELKEY     475
#define ERR_INVITEONLYCHAN    473
#define ERR_CHANNELISFULL     471
// Reply codes
#define RPL_INVITING          341
#define RPL_NOTOPIC           331
#define RPL_TOPIC             332
#define ERR_NOSUCHCHANNEL    403


class Server
{
private:
    int                         _serverFd;
    int                         _port;
    std::string                 _password;
    std::vector<struct pollfd>  _pfds;
    std::map<int, Client>       _clients;
    static bool                 _running;
    void    initServer();
    void    acceptClient();
    void            addPollFd(int fd);
    static void     signalHandler(int signum);
    Server(const Server &other);
    Server &operator=(const Server &other);
    // Bir client'tan veri okur, buffer'a ekler ve tam satırları ayıklar.
    void receiveFromClient(int fd);
    // Client send buffer'ındaki veriyi gönderir.
    void sendToClient(int fd);
    // Client'ı poll listesinden ve map'ten temizler.
    void removeClient(int fd);
    // Client buffer içinden tamamlanmış satırları çıkarır.
    // \r\n ve \n sonlandırıcılarını destekler.
    std::vector<std::string> extractLines(Client& client);
    std::map<std::string, Channel> _channels;

    // Verilen nick'e sahip client'ı bulur, yoksa NULL döner.
    Client* findClientByNick(const std::string& nick);

public:
    Server(int port, const std::string &password);
    ~Server();
    void    run();

    // --- Kanal Komutları ---
    // client'ı params'da belirtilen kanala(lara) katar.
    // params: "#kanal1,#kanal2 şifre1,şifre2" formatını destekler.
    void joinChannel(Client& client, const std::string& params);
    // KICK komutunu işler.
    void handleKick(Client& sender, const std::string& channelName,
                    const std::string& targetNick, const std::string& reason);
    void executeKick(Client& sender, Channel& ch, Client& target,
                     const std::string& channelName,
                     const std::string& targetNick, const std::string& reason);
    // MODE komutunu işler.
    void handleMode(Client& sender, const std::string& channelName,
                    const std::string& modeStr,
                    const std::vector<std::string>& modeParams);
    void applyModeI(Channel& ch, bool adding,
                    std::string& appliedModes, std::string& appliedParams);
    void applyModeT(Channel& ch, bool adding,
                    std::string& appliedModes, std::string& appliedParams);
    bool applyModeK(Client& sender, Channel& ch, bool adding,
                    const std::string& channelName,
                    const std::vector<std::string>& modeParams, size_t& paramIdx,
                    std::string& appliedModes, std::string& appliedParams);
    bool applyModeL(Client& sender, Channel& ch, bool adding,
                    const std::string& channelName,
                    const std::vector<std::string>& modeParams, size_t& paramIdx,
                    std::string& appliedModes, std::string& appliedParams);
    bool applyModeO(Client& sender, Channel& ch, bool adding,
                    const std::string& channelName,
                    const std::vector<std::string>& modeParams, size_t& paramIdx,
                    std::string& appliedModes, std::string& appliedParams);
    // client'ı params'da belirtilen kanaldan çıkarır.
    // params: "#kanal :ayrılma mesajı" formatını destekler.
    void partChannel(Client& client, const std::string& params);
    // TOPIC komutu: kanal topic görüntüleme / değiştirme
    void topicCommand(Client& client, const std::string& params);
    // INVITE komutu: bir kullanıcıyı kanala davet et
    // params: "<nick> <channel>"
    void inviteCommand(Client& client, const std::string& params);
    // Client'a hata mesajı gönderir.
    void sendError(Client& client, int code, const std::string& msg);
    // PRIVMSG komutunu işler.
    // params: "hedef :mesaj" formatındadır.
    void sendServerMessage(Client& client, const std::string& params);
    // JOIN sonrası RPL_NAMREPLY (353) ve RPL_ENDOFNAMES (366) gönderir.
    void sendNamesList(Client& client, Channel& channel);
    // NICK değişikliğini kullanıcının bulunduğu tüm kanallara duyurur.
    // Her kullanıcıya yalnızca bir kez gönderir (deduplicate).
    void broadcastNickChange(Client& client, const std::string& oldNick);
    // Accessors for helpers / command handlers
    std::map<int, Client>          &getClients()  { return _clients; }
    std::map<std::string, Channel> &getChannels() { return _channels; }
};

#endif