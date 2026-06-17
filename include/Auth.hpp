#ifndef AUTH_HPP
#define AUTH_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cctype>
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

void handle_pass(Client &client, const std::vector<std::string> &args,
                 const std::string &serverPassword);

void handle_nick_registration(Client &client, const std::vector<std::string> &args,
                              const std::map<int, Client> &clients);

void handle_user(Client &client, const std::vector<std::string> &args);

bool handle_ping(Client &client, const std::vector<std::string> &args,
                 const std::string &params);

bool handle_quit(Client &client, const std::string &params);

bool handle_nick(Client &client, const std::vector<std::string> &args,
                 const std::map<int, Client> &clients);

bool handle_join(Client &client, const std::vector<std::string> &args,
                 Server &server);

bool handle_part(Client &client, const std::vector<std::string> &args,
                 const std::string &params, Server &server);

bool handle_topic(Client &client, const std::vector<std::string> &args,
                  const std::string &params, Server &server);

bool handle_invite(Client &client, const std::vector<std::string> &args,
                   const std::string &params, Server &server);

bool handle_kick(Client &client, const std::vector<std::string> &args, Server &server);

bool handle_mode(Client &client, const std::vector<std::string> &args, Server &server);

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