#include <string>
#ifndef CLIENT_HPP
#define CLIENT_HPP

class Client
{
private:
    int         _fd;
    std::string _nickname;
    std::string _username;
    bool        _registered;
    std::string _buffer;
public:
    int getFd() const;
    void appendToBuffer(const std::string& data);
    std::string& getBuffer();
};

Client::Client(/* args */)
{
}

Client::~Client()
{
}


#endif