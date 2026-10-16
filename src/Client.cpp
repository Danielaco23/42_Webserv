#include "../includes/Server.hpp"
#include "../includes/Client.hpp"


Client::Client() : fd(-1), server_index(-1),
    chunkedPos(0), chunkedSize(0),
    chunkedReceived(0), chunkedFinished(false),
    chunkedState(CHUNK_SIZE), state(READING)
{
    cgiInputPos = 0;
    last_activity = std::time(NULL);
}

Client::Client(int fd) : fd(fd), server_index(-1),
    chunkedPos(0), chunkedSize(0),
    chunkedReceived(0), chunkedFinished(false),
    chunkedState(CHUNK_SIZE), state(READING)
{
    cgiInputPos = 0;
    last_activity = std::time(NULL);
}

Client::~Client()
{
}