// #include "./include/Server.hpp"
// #include "./include/Client.hpp"
// #include "./include/Channel.hpp"

#include <iostream>
#include <stdexcept>

void check_parameters(int argc, char **argv)
{
    if (argc != 3) 
    {
        throw std::invalid_argument("Usage: ./ircserv <port> <password>");
    }
}

int main(int argc, char **argv)
{
    try
    {
        check_parameters(argc, argv);
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}