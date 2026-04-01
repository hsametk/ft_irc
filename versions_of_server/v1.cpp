// //#include "./include/Server.hpp"
// // #include "./include/Client.hpp"
// // #include "./include/Channel.hpp"

// #include <iostream>
// #include <stdexcept>

// void check_parameters(int argc, char **argv)
// {
//     if (argc != 3) 
//     {
//         throw std::invalid_argument("Usage: ./ircserv <port> <password>");
//     }
// }

// int main(int argc, char **argv)
// {
//     try
//     {
//         check_parameters(argc, argv);
//     }
//     catch(const std::exception& e)
//     {
//         std::cerr << "Error: " << e.what() << '\n';
//         return 1;
//     }
    
//     return 0;
// }
#include <iostream>
#include <vector> //-> for vector
#include <sys/socket.h> //-> for socket()
#include <sys/types.h> //-> for socket()
#include <netinet/in.h> //-> for sockaddr_in
#include <fcntl.h> //-> for fcntl()
#include <unistd.h> //-> for close()
#include <arpa/inet.h> //-> for inet_ntoa()
#include <poll.h> //-> for poll()
#include <csignal> //-> for signal()
#include <map>
#include <string>

int main()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    // AF_INET: IPv4, SOCK_STREAM: TCP, 0: default protocol
    if (socket_fd == -1)
    {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); //-> zero out the structure
    server_addr.sin_family = AF_INET;
    // htons: host to network short, converts port number to network byte order
    server_addr.sin_port = htons(4480);
    // INADDR_ANY: bind to all interfaces
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // bind: associate the socket with the address and port
    if (bind(socket_fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cerr << "Error binding socket" << std::endl;
        return 1;
    }
    // listen: mark the socket as a passive socket that will be used to accept incoming connection requests
    if (listen(socket_fd, 5) == -1)
    {
        std::cerr << "Error listening on socket" << std::endl;
        return 1;
    }
    // accept: block until a client connects, then return a new socket file descriptor for the connection
    int client_socket;
    if ((client_socket = accept(socket_fd, nullptr, nullptr)) == -1)
    {
        std::cerr << "Error accepting connection" << std::endl;
        return 1;
    }
    char buffer[1024] = {0};
    // recv: receive data from the client socket, store it in the buffer, and print it
    size_t value;
    if ((value = recv(client_socket, buffer, sizeof(buffer), 0)) == -1)
    {
        std::cerr << "Error receiving data" << std::endl;
        return 1;
    }
    else if (value == 0)
    {
        std::cout << "Client disconnected" << std::endl;
        close(client_socket);
        return 0;
    }
    else if (value > 0)
    {
         buffer[value] = '\0'; //-> null-terminate the buffer
    }
    std::cout << "Received: " << buffer << std::endl;
    // close: close the client socket and the server socket
    close(client_socket);
    close(socket_fd);
    return 0;
}

// Neden accept() sonrası yeni bir fd geliyor, neden eski socket_fd ile konuşmuyoruz?
// socket_fd = server’ın dinleme socketi
// accept() sonrası dönen fd = o client ile konuşma socketi

// Yani server’ın ana socketi sürekli yeni bağlantıları beklemek için durur.
// Her bağlanan kullanıcı için ayrı bir konuşma kapısı açılmış gibi düşünebilirsin.

// Senin dediğin “her client tek tek dinlenir” fikri doğru yöne gidiyor.



// recv() neden her zaman tam komutu vermek zorunda değil?
// Yani karşı taraf:

// NICK samet\r\n

// gönderse bile sen bunu tek parçada almak zorunda değilsin.

// Bazen şöyle gelebilir:

// NI
// CK sam
// et\r
// \n

// veya birden fazla komut tek recv() içinde de gelebilir.

// Burada küçük düzeltme:

// Sen \n dedin ama IRC’de pratikte satır sonu \r\n olarak düşünülmeli.
// Bu önemli ayrıntı.


// Client bağlantıyı kapatırsa recv() ne döndürür?
// recv() == 0 → karşı taraf bağlantıyı kapattı

// Bu çok önemli bilgi. ft_irc’de client çıkışını anlamak için işine yarayacak.


// server socket’i poll() listesine koyarız çünkü yeni client bağlantı isteği önce server socket üzerinde görünür.

// Yani:

// server socket → “yeni biri bağlanmak istiyor mu?”
// client socket → “bu bağlanan kullanıcı veri gönderdi mi / çıktı mı?”