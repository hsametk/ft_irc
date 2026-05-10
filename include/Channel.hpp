#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include <set>
#include "Client.hpp"

class Channel
{
private:
    std::string             _name;
    std::string             _topic;
    std::string             _key;       // +k modu: kanal şifresi
    int                     _limit;     // +l modu: max üye sayısı (0 = sınırsız)
    bool                    _inviteOnly; // +i modu
    bool                    _topicOpOnly; // +t modu
    // Client'lar Server'a ait olduğu için pointer tutuyoruz (deep copy yok)
    std::map<int, Client*>  _members;   // fd -> Client*
    std::set<int>           _operators; // operator fd'leri
    std::set<int>           _invited;   // INVITE ile davet edilenler (invite-only kanallar için)

public:
    Channel();
    explicit Channel(const std::string &name);

    // Getters
    const std::string &getName()  const;
    const std::string &getTopic() const;
    const std::string &getKey()   const;
    int                getLimit() const;
    bool               isInviteOnly() const;
    bool               isTopicOpOnly() const;

    // Üye yönetimi
    void addMember(Client *client, bool op = false);
    void removeMember(int fd);
    bool hasMember(int fd) const;
    bool isOperator(int fd) const;
    bool isInvited(int fd) const;

    // Operator yönetimi (KICK / MODE / INVITE için)
    void addOperator(int fd);
    void removeOperator(int fd);

    // Invite list yönetimi (INVITE komutu için)
    void addInvited(int fd);
    void removeInvited(int fd);
    bool isInvited(int fd) const;

    const std::map<int, Client*> &getMembers() const;

    void inviteUser(int fd);
    void removeInvited(int fd);

    // Setter'lar (MODE için)
    void setTopic(const std::string &topic);
    void setKey(const std::string &key);
    void setLimit(int limit);
    void setInviteOnly(bool val);
    void setTopicOpOnly(bool val);

    // Tüm üyelere mesaj yayınla
    void broadcast(const std::string &msg, int excludeFd = -1) const;

    void addOperator(int fd);
    void removeOperator(int fd);
};

#endif