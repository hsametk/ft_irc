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
        ~Client();

        int getFd() const;
        void setFd(int fd);

        void appendToBuffer(const std::string& data);
        std::string& getBuffer();
        const std::string& getBuffer() const;
};

#endif