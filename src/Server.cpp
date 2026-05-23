#include "../includes/Server.hpp"
#include "../includes/Client.hpp"




// ============================
// CONSTRUCTOR / DESTRUCTOR
// ============================

Server::Server(int port)
    : _server_fd(-1), _port(port)
{
    _number_of_clients = 0;
    std::memset(&_address, 0, sizeof(_address));
}

Server::~Server()
{
    if (_server_fd != -1)
        close(_server_fd);
}

// ============================
// INIT SOCKET
// ============================

void Server::initSocket()
{
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(_port);

    if (bind(_server_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(_server_fd, 128) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    fcntl(_server_fd, F_SETFL, O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = _server_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    _fds.push_back(pfd);

    std::cout << "Server running on port " << _port << std::endl;
}

// ============================
// RUN LOOP
// ============================

void Server::run()
{
    while (true)
    {
        if (_fds.empty())
            continue;

        if (poll(&_fds[0], _fds.size(), -1) < 0)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end(); )
        {
            int fd = it->fd;
            short revents = it->revents;

            if (revents & POLLIN)
            {
                if (fd == _server_fd)
                {
                    acceptClient();
                    ++it;
                    continue;
                }
                else
                    handleClientRead(fd);
            }

            if (revents & POLLOUT)
            {
                handleClientWrite(fd);
                it->events = POLLIN;
            }

            if (revents & (POLLERR | POLLHUP))
            {
                removeClient(fd);
                it = _fds.begin(); // reinicio seguro
                continue;
            }

            ++it;
        }
    }
}

// ============================
// ACCEPT CLIENT
// ============================

void Server::acceptClient()
{
    int fd = accept(_server_fd, NULL, NULL);
    if (fd < 0)
        return;

    fcntl(fd, F_SETFL, O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    _fds.push_back(pfd);
    _clients.insert(std::make_pair(fd, Client(fd)));

    std::cout << "Client connected: " << fd << std::endl;
}

// ============================
// READ CLIENT
// ============================

void Server::handleClientRead(int fd)
{
    char buffer[1024];

    int bytes = recv(fd, buffer, 1023, 0);

    if (bytes <= 0)
    {
        removeClient(fd);
        return;
    }

    buffer[bytes] = '\0';

    std::map<int, Client>::iterator it = _clients.find(fd);

    if (it == _clients.end())
        return;

    Client &c = it->second;

    c.readBuffer += buffer;

    c.request._req = c.readBuffer;
    c.request._client_fd = fd;

    // MOCK CGI TEST
    c.request._method = "GET";
    c.request._path = "/cgi-bin/test.py";
    c.request._query_string = "";
    c.request._body = "";

    if (handle_cgi_request(*this, c.request))
    {
        return;
    }

    c.writeBuffer =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Not a CGI request";

    c.state = WRITING;

    for (size_t i = 0; i < _fds.size(); i++)
    {
        if (_fds[i].fd == fd)
        {
            _fds[i].events = POLLOUT;
            break;
        }
    }
}

// ============================
// WRITE CLIENT
// ============================

void Server::handleClientWrite(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    Client &c = it->second;

    ssize_t sent = send(fd, c.writeBuffer.c_str(), c.writeBuffer.size(), 0);
    if (sent <= 0)
    {
        removeClient(fd);
        return;
    }

    removeClient(fd);
}

// ============================
// REMOVE CLIENT
// ============================

void Server::removeClient(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it != _clients.end())
        _clients.erase(it);

    close(fd);

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
