#include <string>
#ifndef CLIENT.HPP
#define CLIENT.HPP

class Client
{
private:
    int fd;
    std::string nickaname;
    std::string username;
    bool registered;

public:
    Client(/* args */);
    ~Client();
};

Client::Client(/* args */)
{
}

Client::~Client()
{
}


#endif