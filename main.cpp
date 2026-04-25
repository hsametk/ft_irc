#include "include/Server.hpp"
#include <cstdlib>


void check_args(int argc, char *argv[], int &port, std::string &password)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
        exit(1);
    }
    // check if port is valid
    // if not print error and exit
    // 65535 is max port number
    // 0 is not valid port number
    // 1-1023 are reserved ports
    // 1024-65535 are valid ports
    port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: invalid port number\n";
        exit(1);
    }
    password = argv[2];
    if (password.empty())
    {
        std::cerr << "Error: password cannot be empty\n";
        exit(1);
    }
}
//TODO: Bir servera bağlıyken tekrar tekrar bağlanaıyor bug fix
int main(int argc, char *argv[])
{
    int port;
    std::string password;
    check_args(argc, argv, port, password);
    try
    {
        Server server(port, password);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}