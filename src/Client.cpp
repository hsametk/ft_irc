#include "../include/Client.hpp"

Client::Client() : _fd(-1), _nickname(""), _username(""), _realname(""), _passOk(false),
 _nickset(false), _userset(false), _registered(false), _buffer(""), _sendBuffer("")
{
}

Client::Client(int fd) : _fd(fd), _nickname(""), _username(""), _realname(""),
 _passOk(false), _nickset(false), _userset(false), _registered(false), _buffer(""), _sendBuffer("")
{
}
Client::Client(const Client &other)
    : _fd(other._fd), _nickname(other._nickname), _username(other._username),
      _realname(other._realname), _passOk(other._passOk), _nickset(other._nickset),
      _userset(other._userset), _registered(other._registered), _buffer(other._buffer),
      _sendBuffer(other._sendBuffer)
{
}

Client &Client::operator=(const Client &other)
{
    if (this != &other)
    {
        _fd         = other._fd;
        _nickname   = other._nickname;
        _username   = other._username;
        _realname   = other._realname;
        _passOk     = other._passOk;
        _nickset    = other._nickset;
        _userset    = other._userset;
        _registered = other._registered;
        _buffer     = other._buffer;
        _sendBuffer = other._sendBuffer;
    }
    return *this;
}
Client::~Client()
{
}

int Client::getFd() const
{
    return _fd;
}
void Client::setFd(int fd)
{
    _fd = fd;
}

void Client::appendToBuffer(const std::string& data)
{
    _buffer.append(data);
}
const std::string& Client::getBuffer() const
{
    return _buffer;
}
std::string& Client::getBuffer()
{
    return _buffer;
}

void Client::sendMessage(const std::string &msg)
{
    _sendBuffer.append(msg);
}

std::string& Client::getSendBuffer()
{
    return _sendBuffer;
}

void Client::eraseFromSendBuffer(size_t n)
{
    if (n <= _sendBuffer.size())
        _sendBuffer.erase(0, n);
    else
        _sendBuffer.clear();
}

// Getters
bool Client::isPassOk()    const { return _passOk; }
bool Client::isNickSet()   const { return _nickset; }
bool Client::isUserSet()   const { return _userset; }
bool Client::isRegistered()const { return _registered; }

// Setters
void Client::setPassOk(bool v)    { _passOk     = v; }
void Client::setNickSet(bool v)   { _nickset    = v; }
void Client::setUserSet(bool v)   { _userset    = v; }
void Client::setRegistered(bool v){ _registered = v; }

const std::string& Client::getNickname() const { return _nickname; }
void Client::setNickname(const std::string& nick) { _nickname = nick; }

const std::string& Client::getUsername() const { return _username; }
void Client::setUsername(const std::string& user) { _username = user; }