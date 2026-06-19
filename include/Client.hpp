#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <sys/socket.h>
#include <unistd.h>

class Client
{
    private:
        int         _fd;
        std::string _nickname;
        std::string _username;
        std::string _realname;
        bool        _passOk;
        bool        _nickset;
        bool        _userset;
        bool        _registered;
        // Client'tan gelen ham veri burada tutulur.
        // recv() partial veri döndürebileceği için komut hemen parse edilmez.
        // Veri burada biriktirilir, tam satır gelince ayrılır.
        std::string _buffer;
        std::string _sendBuffer;

    public:
        Client();
        Client(int fd);
        Client(const Client &other);
        Client &operator=(const Client &other);
        ~Client();
        // Client socket fd bilgisini döndürür.
        int getFd() const;
        // Client socket fd bilgisini ayarlar.
        void setFd(int fd);
        // Yeni bool user ile ilgili olan değişkenler için getter/setter
        bool isPassOk()    const;
        bool isNickSet()   const;
        bool isUserSet()   const;
        bool isRegistered()const;
        void setPassOk(bool v);
        void setNickSet(bool v);
        void setUserSet(bool v);
        void setRegistered(bool v);
        // Nickname getter/setter
        const std::string& getNickname() const;
        void               setNickname(const std::string& nick);
        // Username getter/setter
        const std::string& getUsername() const;
        void               setUsername(const std::string& user);
        // Realname getter/setter
        const std::string& getRealname() const;
        void               setRealname(const std::string& realname);
        // Gelen veriyi client buffer'ına ekler.
        void appendToBuffer(const std::string& data);
        // Buffer üzerinde değişiklik yapabilmek için referans döndürür.
        std::string& getBuffer();
        // Sadece okumalık const erişim sağlar.
        const std::string& getBuffer() const;
        void sendMessage(const std::string& msg);
        // Giden veri buffer'a eklenir ve poll ile gönderilir.
        std::string& getSendBuffer();
        void eraseFromSendBuffer(size_t n);
};

#endif