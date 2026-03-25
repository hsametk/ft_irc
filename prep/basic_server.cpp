#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#define PORT 8080

int main()
{
    int server_fd;
    int client_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket failed");
        return EXIT_FAILURE;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    std::cout << "Server " << PORT << " portunda dinliyor..." << std::endl;
    std::cout << "Bir client baglanmasi bekleniyor..." << std::endl;

    client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (client_fd < 0)
    {
        perror("accept failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    std::cout << "Bir client baglandi." << std::endl;

    while (true)
    {
        std::memset(buffer, 0, sizeof(buffer));

        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read < 0)
        {
            perror("recv failed");
            break;
        }
        else if (bytes_read == 0)
        {
            std::cout << "Client baglantiyi kapatti." << std::endl;
            break;
        }

        buffer[bytes_read] = '\0';

        std::string message(buffer);
        std::cout << "Message from client: [" << message << "]" << std::endl;

        if (message.find("QUIT") != std::string::npos)
        {
            std::cout << "QUIT komutu alindi. Server kapaniyor..." << std::endl;
            break;
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}