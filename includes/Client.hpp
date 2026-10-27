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

enum ChunkedState
{
    CHUNK_SIZE,
    CHUNK_DATA,
    CHUNK_CRLF,
    CHUNK_TRAILERS,
    CHUNK_DONE
};

class Client
{
public:
    int fd;
    int server_index; // <- IMPORTANTE: qué server lo aceptó

    std::string readBuffer;
    std::string writeBuffer;

    size_t chunkedPos;
    size_t chunkedSize;
    size_t chunkedReceived;
    bool chunkedFinished;
    ChunkedState chunkedState;

    ClientState state;

    HttpRequest request;

    std::string cgiOutput;
    std::string cgiResponseHeader;
    size_t cgiInputPos;
    size_t cgiBodyStart;
    size_t cgiOutputPos;
    size_t cgiResponseHeaderPos;
    bool cgiWriting;

    time_t last_activity; 

    Client();
    Client(int fd);
    ~Client();
};

#endif