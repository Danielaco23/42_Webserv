#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "Request.hpp" //http parse temp necesito algo con lo q chambear

/**
 * @brief Represents the current state of a client connection.
 */
enum ClientState
{
    READING,  /**< Client is reading incoming data (waiting for request). */
    WRITING,  /**< Client is ready to send a response. */
    CLOSED    /**< Client connection is closed. */
};

/**
 * @brief Represents a connected client in the server.
 * 
 * This class stores all the necessary information to manage
 * a client connection, including:
 * - Its socket file descriptor
 * - Buffers for incoming and outgoing data
 * - Its current state in the communication lifecycle
 */
class Client
{
    public:

        int fd; // File descriptor associated with the client socket.

        std::string readBuffer; //Buffer used to store incoming data from the client.
        std::string writeBuffer; // Buffer used to store outgoing data to be sent to the client.
        ClientState state;
        /**< Current state of the client (READING, WRITING, CLOSED). */
        /**
         * @brief Default constructor.
         * 
         * Initializes the client with an invalid file descriptor
         * and sets the state to READING.
         */
        Client() : fd(-1), state(READING) {}

        /**
         * @brief Constructs a client with a given file descriptor.
         * 
         * @param fd File descriptor of the client socket.
         */
        Client(int fd);
        Request request;
};

#endif