#include "../include/Client.hpp"

// TODO::temporary classes for execute
Client::Client()
    : _fd(-1), _nickname(""), _username(""), _registered(false), _buffer("")
{
}

Client::Client(int fd)
    : _fd(fd), _nickname(""), _username(""), _registered(false), _buffer("")
{
}

Client::Client(const Client &other)
    : _fd(other._fd), _nickname(other._nickname), _username(other._username),
      _registered(other._registered), _buffer(other._buffer)
{
}

Client &Client::operator=(const Client &other)
{
    if (this != &other)
    {
        _fd         = other._fd;
        _nickname   = other._nickname;
        _username   = other._username;
        _registered = other._registered;
        _buffer     = other._buffer;
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

std::string &Client::getBuffer()
{
    return _buffer;
}

void Client::appendToBuffer(const std::string &data)
{
    _buffer += data;
}
