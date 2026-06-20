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
// INIT VARIABLES
// ============================

void Server::initVariables()
{
    this->_number_of_clients = 0;
    this->_request_data._method = "";
    this->_request_data._file_path = "";
    this->_request_data._req = "";
    this->_request_data._request_id = "";
    this->_request_data._path = "";
    this->_request_data._version = "";
    this->_request_data._query_string = "";
    this->_request_data._body = "";
    this->_request_data._www_root = "";
    this->_request_data._client_fd = -1;
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
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(_port);

    if (bind(_server_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

// ============================
// START LISTENING
// ============================
void Server::startListening()
{
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
    std::cout << "Try accessing http://localhost:" << _port << " in your browser." << std::endl;
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

    if (!check_response(*this, c.request))
    {
        removeClient(fd);
        return;
    }

    if (!parse_request_line(c.request))
    {
        send_error_page(fd, 400, "Bad Request", "Malformed request line.", c.request._request_id);
        removeClient(fd);
        return;
    }

    if (is_cgi_path(c.request._path))
    {
        if (!handle_cgi_request(*this, c.request))
            send_error_page(fd, 500, "Internal Server Error", "CGI handler failed.", c.request._request_id);
        removeClient(fd);
        return;
    }

    if (c.request._method == "POST")
    {
        this->handle_post_upload(fd, c.request._path, c.request._request_id, c.request._req, c.request._www_root);
        removeClient(fd);
        return;
    }

    if (c.request._method == "DELETE")
    {
        std::string fullPath =
            c.request._www_root.empty()
                ? std::string("www") + c.request._path
                : c.request._www_root + c.request._path;

        if (std::remove(fullPath.c_str()) == 0)
            send_error_page(fd, 204, "No Content", "", c.request._request_id);
        else
            send_error_page(fd, 404, "Not Found", "File not found.", c.request._request_id);

        removeClient(fd);
        return;
    }

    if (c.request._method != "GET" && c.request._method != "HEAD")
    {
        send_error_page(fd, 405, "Method Not Allowed", "Only GET, POST and DELETE supported.", c.request._request_id);
        removeClient(fd);
        return;
    }

    if (c.request._path == "/uploads")
    {
        this->handle_uploads_listing(fd, c.request._www_root);
        removeClient(fd);
        return;
    }

    std::string normalized_path;
    if (c.request._path == "/")
        normalized_path = "/index.html";
    else
        normalized_path = c.request._path;

    if (normalized_path.find("..") != std::string::npos)
    {
        send_error_page(fd, 400, "Bad Request", "Invalid path.", c.request._request_id);
        removeClient(fd);
        return;
    }

    std::string base_path;
    if (c.request._www_root.empty())
        base_path = "www";
    else
        base_path = c.request._www_root;

    c.request._file_path = base_path + normalized_path;

    struct stat st;
    if (stat(c.request._file_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        c.request._file_path += "/index.html";
    else if (stat(c.request._file_path.c_str(), &st) != 0 && normalized_path.find('.') == std::string::npos)
        c.request._file_path = base_path + normalized_path + "/index.html";

    send_file(fd, c.request._file_path, c.request._request_id);
    removeClient(fd);
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

    if (fcntl(fd, F_GETFD) != -1)
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
