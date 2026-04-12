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

        // Client'tan gelen ham veri burada tutulur.
        // recv() partial veri döndürebileceği için komut hemen parse edilmez.
        // Veri burada biriktirilir, tam satır gelince ayrılır.
        std::string _buffer;

    public:
        Client();
        Client(int fd);
        ~Client();

        // Client socket fd bilgisini döndürür.
        int getFd() const;

        // Client socket fd bilgisini ayarlar.
        void setFd(int fd);

        // Gelen veriyi client buffer'ına ekler.
        void appendToBuffer(const std::string& data);

        // Buffer üzerinde değişiklik yapabilmek için referans döndürür.
        std::string& getBuffer();

        // Sadece okumalık const erişim sağlar.
        const std::string& getBuffer() const;

};

#endif