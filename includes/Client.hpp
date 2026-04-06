#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

enum ClientState
{
    READING,
    WRITING,
    CLOSED
};

class Client
{
    public:
        int     fd;
        std::string readBuffer;
        std::string writeBuffer;
        ClientState state;

        Client() : fd(-1), state(READING) {}
        Client(int fd);
};

#endif