#include "../includes/Server.hpp"
#include "../includes/Client.hpp"

Client::Client() : fd(-1), server_index(-1), state(READING)
{
    
}

Client::Client(int fd) : fd(fd), server_index(-1), state(READING)
{
}

Client::~Client()
{
}