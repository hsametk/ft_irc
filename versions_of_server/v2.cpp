#include <iostream>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>

int main()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        std::cerr << "Error creating socket\n";
        return 1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4480);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cerr << "Error binding socket\n";
        return 1;
    }

    if (listen(socket_fd, 5) == -1)
    {
        std::cerr << "Error listening\n";
        return 1;
    }

    pollfd pfds[1024];
    pfds[0].fd = socket_fd;
    pfds[0].events = POLLIN;

    int nfds = 1;

    while (true)
    {
        int poll_count = poll(pfds, nfds, -1);
        if (poll_count == -1)
        {
            std::cerr << "Poll error\n";
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            if (pfds[i].revents & POLLIN)
            {
                // NEW CONNECTION
                if (pfds[i].fd == socket_fd)
                {
                    int client_fd = accept(socket_fd, NULL, NULL);
                    if (client_fd == -1)
                    {
                        std::cerr << "Accept error\n";
                        continue;
                    }

                    if (nfds >= 1024)
                    {
                        std::cerr << "Max clients reached\n";
                        close(client_fd);
                        continue;
                    }

                    pfds[nfds].fd = client_fd;
                    pfds[nfds].events = POLLIN;
                    nfds++;

                    std::cout << "New client connected: " << client_fd << "\n";
                }
                // CLIENT DATA
                else
                {
                    char buffer[1025];
                    ssize_t bytes = recv(pfds[i].fd, buffer, 1024, 0);

                    if (bytes == -1)
                    {
                        std::cerr << "Recv error\n";
                        close(pfds[i].fd);
                        pfds[i] = pfds[nfds - 1];
                        nfds--;
                        i--;
                    }
                    else if (bytes == 0)
                    {
                        std::cout << "Client disconnected: " << pfds[i].fd << "\n";
                        close(pfds[i].fd);
                        pfds[i] = pfds[nfds - 1];
                        nfds--;
                        i--;
                    }
                    else
                    {
                        buffer[bytes] = '\0';
                        std::cout << "Received: " << buffer;
                    }
                }
            }
        }
    }

    // cleanup (normalde signal ile çıkarsın)
    for (int i = 0; i < nfds; i++)
        close(pfds[i].fd);

    return 0;
}