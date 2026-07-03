#pragma once

#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include "HttpRequest.hpp"


enum ClientState
{
    READING,
    WRITING,
    CLOSED
};

class Client
{
public:
    int fd;
    int server_index; // <- IMPORTANTE: qué server lo aceptó

    std::string readBuffer;
    std::string writeBuffer;

    ClientState state;

    HttpRequest request;

    time_t last_activity; 

    Client();
    Client(int fd);
    ~Client();
};

#endif