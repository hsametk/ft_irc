#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
private:
    int         _fd;
    std::string _nickname;
    std::string _username;
    bool        _registered;
    std::string _buffer;

public:
    Client();
    Client(int fd);
    Client(const Client &other);
    Client &operator=(const Client &other);
    ~Client();

    int          getFd() const;
    std::string &getBuffer();
    void         appendToBuffer(const std::string &data);
};

#endif