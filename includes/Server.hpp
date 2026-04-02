#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <map>
#include <poll.h>
#include <vector>
#include <fcntl.h>
#include <netinet/in.h>
#include <cstdlib>  // exit, EXIT_FAILURE
#include <cstdio>   // perror
#include <cstring> //memset
#include <unistd.h> //close
#include <arpa/inet.h> //socket, bind , listen, accept
#include "Client.hpp"

class Server
{
    private:
        int                 _server_fd; // fd sockeer
        int                 _port;
        struct sockaddr_in  _address; //ip, port, Af_inet

        std::vector<struct pollfd>_fds;
        std::map<int, Client> _clients;
    
    public:
        Server(int port);
        ~Server();

        void initSocket();
        void run();

    private:
        void acceptClient();
        void handleClientRead(int fd);
        void handleClientWrite(int fd);
        void removeClient(int fd);

};

#endif