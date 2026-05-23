#include "../includes/Server.hpp"
#include "../includes/Client.hpp"

Client::Client() : fd(-1), state(READING)
{
    readBuffer = "";
    writeBuffer = "";
}

Client::Client(int fd) : fd(fd), state(READING)
{
    readBuffer = "";
    writeBuffer = "";
}

Client::~Client()
{
}