#include "Server.hpp"#include "../include/Client.hpp"

Client::Client() : _fd(-1), _nickname(""), _username(""), _registered(false), _buffer("")
{
}

Client::Client(int fd) : _fd(fd), _nickname(""), _username(""), _registered(false), _buffer("")
{
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
    _buffer += data;
}

std::string& Client::getBuffer()
{
    return _buffer;
}

const std::string& Client::getBuffer() const
{
    return _buffer;
}