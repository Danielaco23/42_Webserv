#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>     // std::cout, std::cerr
#include <string>       // std::string
#include <map>          // std::map (client storage)
#include <poll.h>       // poll(), struct pollfd
#include <vector>       // std::vector (fd list)
#include <fcntl.h>      // fcntl() (non-blocking mode)
#include <netinet/in.h> // sockaddr_in, htons(), INADDR_ANY
#include <cstdlib>      // exit(), EXIT_FAILURE
#include <cstdio>       // perror()
#include <cstring>      // memset()
#include <unistd.h>     // close()
#include <arpa/inet.h>  // socket(), bind(), listen(), accept()
#include "Client.hpp"
#include "CGI.hpp"  //cuestion en pruebas

#include <fstream>
#include <sstream>

/**
 * @brief Represents a simple HTTP server using non-blocking sockets and poll().
 * 
 * This class is responsible for:
 * - Creating and configuring a server socket
 * - Accepting multiple client connections
 * - Handling client I/O using poll()
 * - Managing connected clients
 */
class Server
{
    private:
        int                 _server_fd; // Server socket file descriptor (created with socket()).
        int                 _port; // Port on which the server listens for incoming connections.
        struct sockaddr_in  _address; // Structure containing IP address and port (used with bind()).

        std::vector<struct pollfd>_fds; // List of file descriptors monitored by poll()
        std::map<int, Client> _clients; // Maps client file descriptors to Client objects.
    
    public:
        Server(int port);
        ~Server();
          /**
         * @brief Initializes the server socket.
         * 
         * Steps performed:
         * - Creates a socket using socket()
         * - Configures options with setsockopt()
         * - Binds the socket to an address with bind()
         * - Starts listening with listen()
         * - Sets non-blocking mode using fcntl()
         * - Adds the socket to poll() monitoring list
         */
        void initSocket();
        /**
         * @brief Starts the main server loop.
         * 
         * Uses poll() to monitor multiple file descriptors and:
         * - Accepts new clients
         * - Reads incoming data
         * - Sends responses
         * - Handles disconnections and errors
         */
        void run();

    private:
        /**
         * @brief Accepts a new incoming client connection.
         * 
         * Uses accept() to create a new socket for the client,
         * sets it to non-blocking mode, and adds it to poll().
         */
        void acceptClient();

        /**
         * @brief Handles incoming data from a client.
         * 
         * @param fd File descriptor of the client socket.
         * 
         * Uses recv() to read data and stores it in the client buffer.
         * Prepares a response and switches the socket to POLLOUT.
         */
        void handleClientRead(int fd);

        /**
         * @brief Sends data to a client.
         * 
         * @param fd File descriptor of the client socket.
         * 
         * Uses send() to transmit the response to the client.
         */
        void handleClientWrite(int fd);

         /**
         * @brief Removes a client connection.
         * 
         * @param fd File descriptor of the client socket.
         * 
         * Closes the socket using close(), removes it from poll(),
         * and deletes it from the client map.
         */
        void removeClient(int fd);

        /**
         * @brief Sends a web page to the client.
         * 
         * @param client_fd File descriptor of the client socket.
         * 
         * (Planned) Reads a file (e.g., HTML) and sends it as an HTTP response.
         */
		void sendWebPage(int client_fd);

};

#endif