#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <cstdlib>  // exit, EXIT_FAILURE
#include <cstdio>   // perror
#include <cstring> //memset
#include <unistd.h> //close
#include <arpa/inet.h> //socket, bind , listen, accept

#include <fstream>
#include <sstream>
#include <string>

class Server
{
    private:
        int                 _server_fd; // fd sockeer
        int                 _port;
        struct sockaddr_in  _address; //ip, port, Af_inet
    
    public:
        Server(int port);
        ~Server();

        void initSocket();
        void startListening();
        void acceptConnection();

        int getServerFd() const;

		void sendWebPage(int client_fd);

};

#endif