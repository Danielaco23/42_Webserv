#include "../includes/Server.hpp"
/**
 * @brief Constructs a Server object and initializes its address structure.
 * 
 * @param port The port on which the server will listen for incoming connections.
 */
Server::Server(int port) : _server_fd(-1), _port(port)
{
    std::memset(&_address, 0, sizeof(_address));
}

/**
 * @brief Destroys the Server object and closes the server socket if open.
 */
Server::~Server()
{
    if (_server_fd != -1)
        close(_server_fd);
}

/**
 * @brief Initializes the server socket, binds it to the specified port,
 * and starts listening for incoming connections.
 * 
 * This function sets the socket to non-blocking mode and prepares it
 * for polling by adding it to the poll file descriptor list.
 */
void Server::initSocket()
{
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr =  INADDR_ANY; // acepta cualquier IP
    _address.sin_port = htons(_port);

    if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(_server_fd, 100) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Non blocking
    fcntl(_server_fd, F_SETFL, O_NONBLOCK);

    //poll
    struct pollfd pfd;
    pfd.fd = _server_fd;
    pfd.events = POLLIN;
    _fds.push_back(pfd);

    std::cout << "Server running on port " << _port << std::endl;
}

/**
 * @brief Starts the main server loop.
 * 
 * This function continuously monitors file descriptors using poll()
 * and handles incoming connections, client reads, writes, and errors.
 */
void Server::run()
{
    while (true)
    {
        if (poll(&_fds[0], _fds.size(), -1) < 0)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < _fds.size(); i++)
        {
            if (_fds[i].revents & POLLIN)
            {
                if (_fds[i].fd == _server_fd)
                    acceptClient();
                else
                    handleClientRead(_fds[i].fd);
            }

            if (_fds[i].revents & POLLOUT)
            {
                handleClientWrite(_fds[i].fd);
            }
            if (_fds[i].revents & (POLLERR | POLLHUP))
            {
                removeClient(_fds[i].fd);
                continue;
            }
        }
    }
}


/**
 * @brief Accepts a new client connection.
 * 
 * The new client socket is set to non-blocking mode and added to the poll list.
 * A corresponding Client object is also created and stored.
 */
void Server::acceptClient()
{
    int client_fd = accept(_server_fd, NULL, NULL);
    if (client_fd < 0)
    {
        perror("accept failed");
        return;
    }

    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    _fds.push_back(pfd);

    _clients.insert(std::make_pair(client_fd, Client(client_fd)));

    std::cout << "New client connected!" << std::endl;
}

/**
 * @brief Handles incoming data from a client.
 * 
 * @param fd The file descriptor of the client socket.
 * 
 * Reads data from the client, stores it in the client's buffer,
 * and prepares a simple HTTP response. The client's poll event
 * is then switched to POLLOUT for sending the response.
 */

void Server::handleClientRead(int fd)
{
     // read client
    char buffer[1024];

    int bytes_read = recv(fd, buffer, 1023, 0);

    if (bytes_read <= 0)
    {
        removeClient(fd);
        return;
    }

    buffer[bytes_read] = '\0';

    std::map<int, Client>::iterator it = _clients.find(fd);

    if (it == _clients.end())
    {
        std::cout << "Client not found" << std::endl;
        return;
    }

    // MOCK REQUEST TEMPORAL
    it->second.request.method = "GET";
    it->second.request.path = "/cgi-bin/test.py";
    it->second.request.queryString = "";
    it->second.request.body = "";

    it->second.readBuffer += buffer;

    std::cout << "Request from "
              << fd
              << ":\n"
              << it->second.readBuffer
              << std::endl;

    // CGI TEST
    std::string output = CGI::execute(
        "./cgi-bin/test.py",
        "/usr/bin/python3",
        "GET",
        "",
        "",
        std::map<std::string, std::string>()
    );
 // Send HTTP
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n" +
        output;

    it->second.writeBuffer = response;

    it->second.state = WRITING;
    // change event a POLLOUT
    for (size_t i = 0; i < _fds.size(); i++)
    {
        if (_fds[i].fd == fd)
            _fds[i].events = POLLOUT;
    }
}



/**
 * @brief Sends data to a client.
 * 
 * @param fd The file descriptor of the client socket.
 * 
 * Attempts to send the prepared response to the client.
 * If successful, the client connection is closed.
 */
void Server::handleClientWrite(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    Client &client = it->second;

    int sent = send(fd,client.writeBuffer.c_str(),client.writeBuffer.size(), 0);
    if (sent <= 0)
    {
        removeClient(fd);
        return;
    }

    removeClient(fd);
}

/**
 * @brief Removes a client from the server.
 * 
 * @param fd The file descriptor of the client socket.
 * 
 * Closes the socket, removes it from the poll list and
 * deletes its associated Client object.
 */
void Server::removeClient(int fd)
{
    close(fd);
    _clients.erase(fd);

    for (size_t i = 0; i < _fds.size(); i++)
    {
        if (_fds[i].fd == fd)
        {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }

    std::cout << "Client disconnected: " << fd << std::endl;
}

//int Server::getServerFd() const
//{
//    return _server_fd;
//}