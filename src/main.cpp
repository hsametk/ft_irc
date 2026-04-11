#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include <iostream>
#include <vector>

// int main() // geçici test için 
// {
//     Client client(42);
//     Server server;

//     client.appendToBuffer("PASS 12");
//     client.appendToBuffer("34\r\nNICK zu");
//     client.appendToBuffer("onen\r\nUSER zuonen 0 * :Zulal\r\nPARTIAL");

//     std::vector<std::string> lines = server.extractLines(client);

//     std::cout << "Extracted lines:" << std::endl;
//     for (size_t i = 0; i < lines.size(); ++i)
//         std::cout << "[" << lines[i] << "]" << std::endl;

//     std::cout << "Remaining buffer:" << std::endl;
//     std::cout << "[" << client.getBuffer() << "]" << std::endl;

//     return 0;
// }

#include "../include/Server.hpp"

int main()
{
    return 0;
}