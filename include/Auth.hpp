#ifndef AUTH_HPP
#define AUTH_HPP

#include <string>
#include <map>
#include "Client.hpp"

// Kayıtsız client'tan gelen bir IRC satırını işler (PASS, NICK, USER, CAP).
void registration_state(Client &client, const std::string &line,
                        const std::string &serverPassword,
                        const std::map<int, Client> &clients);

// Kayıtlı client'tan gelen bir IRC satırını işler.
// Bağlantının kesilmesi gerekiyorsa true döner (örn. QUIT).
bool dispatch_command(Client &client, const std::string &line,
                      std::map<int, Client> &clients);
// İstemciden gelen mesajı durumuna (kayıtlı/kayıtsız) göre yönlendirir.
// Bağlantının kesilmesi gerekiyorsa true döner.
bool handle_client_message(Client &client, const std::string &line,
                           const std::string &serverPassword,
                           std::map<int, Client> &clients);

#endif
