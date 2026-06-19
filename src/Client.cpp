#include "../include/Client.hpp"

// ---------------------------------------------------------------------------
// Client — sunucuya bağlı tek bir kullanıcıyı temsil eder.
// fd, kayıt durumu (PASS/NICK/USER), kimlik bilgileri ve partial veri için
// kullanılan receive buffer'ı tutar.
// ---------------------------------------------------------------------------

// --- Constructors / Destructor ---
Client::Client()
    : _fd(-1), _nickname(""), _username(""), _realname(""),
      _passOk(false), _nickset(false), _userset(false),
      _registered(false), _buffer("") {}

Client::Client(int fd)
    : _fd(fd), _nickname(""), _username(""), _realname(""),
      _passOk(false), _nickset(false), _userset(false),
      _registered(false), _buffer("") {}

Client::Client(const Client &other)
    : _fd(other._fd), _nickname(other._nickname), _username(other._username),
      _realname(other._realname), _passOk(other._passOk), _nickset(other._nickset),
      _userset(other._userset), _registered(other._registered), _buffer(other._buffer) {}

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
    }
    return *this;
}

Client::~Client() {}

// --- Fd ---
int  Client::getFd() const   { return _fd; }
void Client::setFd(int fd)   { _fd = fd; }

// --- Identity (nick / user / real) ---
const std::string& Client::getNickname() const            { return _nickname; }
void               Client::setNickname(const std::string& nick) { _nickname = nick; }

const std::string& Client::getUsername() const            { return _username; }
void               Client::setUsername(const std::string& user) { _username = user; }

const std::string& Client::getRealname() const            { return _realname; }
void               Client::setRealname(const std::string& real) { _realname = real; }

// --- Registration state ---
bool Client::isPassOk()     const { return _passOk; }
bool Client::isNickSet()    const { return _nickset; }
bool Client::isUserSet()    const { return _userset; }
bool Client::isRegistered() const { return _registered; }

void Client::setPassOk(bool v)     { _passOk     = v; }
void Client::setNickSet(bool v)    { _nickset    = v; }
void Client::setUserSet(bool v)    { _userset    = v; }
void Client::setRegistered(bool v) { _registered = v; }

// --- Receive buffer (partial veri biriktirme) ---
void Client::appendToBuffer(const std::string& data) { _buffer.append(data); }

std::string&       Client::getBuffer()       { return _buffer; }
const std::string& Client::getBuffer() const { return _buffer; }

// --- Outgoing message ---
void Client::sendMessage(const std::string &msg)
{
    send(_fd, msg.c_str(), msg.size(), 0);
}