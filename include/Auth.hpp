#ifndef AUTH_HPP
#define AUTH_HPP

#include <string>
#include <map>
#include "Client.hpp"
#include "Server.hpp"

struct ParsedCommand //zülal
{
    std::string command;
    std::vector<std::string> args;
};

// Line içindeki fazla boşlukları normalize eder.  //zülal
std::string normalize_spaces(const std::string &line);

// IRC satırını command + args olarak parçalar.  //zülal
ParsedCommand parse_line(const std::string &line);


// Kayıtsız client'tan gelen bir IRC satırını işler (PASS, NICK, USER, CAP).
void registration_state(Client &client, const std::string &line,
                        const std::string &serverPassword,
                        const std::map<int, Client> &clients);

// Kayıtlı client'tan gelen bir IRC satırını işler.
// Bağlantının kesilmesi gerekiyorsa true döner (örn. QUIT).
bool dispatch_command(Client &client, const std::string &line,
                      std::map<int, Client> &clients, Server &server);

// İstemciden gelen mesajı durumuna (kayıtlı/kayıtsız) göre yönlendirir.
// Bağlantının kesilmesi gerekiyorsa true döner.
bool handle_client_message(Client &client, const std::string &line,
                           const std::string &serverPassword,
                           std::map<int, Client> &clients, Server &server);

#endif
