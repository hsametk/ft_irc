#include "../include/Channel.hpp"

Channel::Channel()
    : _name(""), _topic(""), _key(""), _limit(0),
      _inviteOnly(false), _topicOpOnly(false) {}

Channel::Channel(const std::string &name)
    : _name(name), _topic(""), _key(""), _limit(0),
      _inviteOnly(false), _topicOpOnly(false) {}

const std::string            &Channel::getName()    const { return _name; }
const std::string            &Channel::getTopic()   const { return _topic; }
const std::string            &Channel::getKey()     const { return _key; }
int                           Channel::getLimit()   const { return _limit; }
bool                          Channel::isInviteOnly()  const { return _inviteOnly; }
bool                          Channel::isTopicOpOnly() const { return _topicOpOnly; }
const std::map<int, Client*> &Channel::getMembers() const { return _members; }

void Channel::setTopic(const std::string &topic) { _topic = topic; }
void Channel::setKey(const std::string &key)     { _key = key; }
void Channel::setLimit(int limit)                { _limit = limit; }
void Channel::setInviteOnly(bool val)            { _inviteOnly = val; }
void Channel::setTopicOpOnly(bool val)           { _topicOpOnly = val; }

void Channel::addMember(Client *client, bool op)
{
    _members[client->getFd()] = client;
    if (op)
        _operators.insert(client->getFd());
}

void Channel::removeMember(int fd)
{
    _members.erase(fd);
    _operators.erase(fd);
    _invited.erase(fd);
}

bool Channel::hasMember(int fd) const
{
    return _members.find(fd) != _members.end();
}

void Channel::addOperator(int fd)
{
    if (hasMember(fd))
        _operators.insert(fd);
}

void Channel::removeOperator(int fd)
{
    _operators.erase(fd);
}

bool Channel::isOperator(int fd) const
{
    return _operators.find(fd) != _operators.end();
}

void Channel::addInvited(int fd)
{
    _invited.insert(fd);
}

void Channel::removeInvited(int fd)
{
    _invited.erase(fd);
}

bool Channel::isInvited(int fd) const
{
    return _invited.find(fd) != _invited.end();
}


void Channel::broadcast(const std::string &msg, int excludeFd) const
{
    for (std::map<int, Client*>::const_iterator it = _members.begin();
         it != _members.end(); ++it)
    {
        if (it->first != excludeFd)
            it->second->sendMessage(msg);
    }
}