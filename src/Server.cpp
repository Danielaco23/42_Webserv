#include "../includes/Server.hpp"

Server::Server(int port) : _server_fd(-1), _port(port)
{
    std::memset(&_address, 0, sizeof(_address));
}

Server::~Server()
{
    if (_server_fd != -1)
        close(_server_fd);
}

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

    it->second.readBuffer += buffer;

    // Send HTTP
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";
    
    std::cout << "Request from " << fd << ":\n" << it->second.readBuffer << std::endl;

    it->second.writeBuffer = response;
    it->second.state = WRITING;

    // change event a POLLOUT
    for (size_t i = 0; i < _fds.size(); i++)
    {
        if (_fds[i].fd == fd)
            _fds[i].events = POLLOUT;
    }
}

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