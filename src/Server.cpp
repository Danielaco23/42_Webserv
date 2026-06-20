#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include <cerrno>

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
    this->_request_data._maxBodySize = 50 * 1024 * 1024;
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

    std::cout << "Server running on port " << this->_port << std::endl;
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
        
        for (size_t i = 0; i < _fds.size(); i++)
        {
            int fd = _fds[i].fd;
            short revents = _fds[i].revents;

            if (revents == 0)
                continue;

            if (revents & POLLIN)
            {
                if (fd == _server_fd)
                    acceptClient();
                else
                    handleClientRead(fd);
            }

            if (revents & POLLOUT)
                handleClientWrite(fd);

            if (revents & (POLLERR | POLLHUP | POLLNVAL))
                _pending_remove.push_back(fd);
        }

        // APPLY NEW CLIENTS
    
        for (size_t i = 0; i < _pending_add.size(); i++)
            _fds.push_back(_pending_add[i]);

        _pending_add.clear();

        // REMOVE CLIENTS

        for (size_t i = 0; i < _pending_remove.size(); i++)
            removeClient(_pending_remove[i]);

        _pending_remove.clear();
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

    _pending_add.push_back(pfd);
    _clients.insert(std::make_pair(fd, Client(fd)));

    std::cout << "Client connected: " << fd << std::endl;
}

// ============================
// READ CLIENT
// ============================

static size_t find_headers_end(const std::string &request)
{
    size_t pos = request.find("\r\n\r\n");
    if (pos != std::string::npos)
        return pos + 4;
    pos = request.find("\n\n");
    if (pos != std::string::npos)
        return pos + 2;
    return std::string::npos;
}

static std::string to_lower_copy(const std::string &value)
{
    std::string lowered = value;
    for (size_t i = 0; i < lowered.size(); ++i)
        lowered[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[i])));
    return lowered;
}

static size_t extract_content_length(const std::string &request)
{
    size_t headers_end = find_headers_end(request);
    if (headers_end == std::string::npos)
        return 0;

    std::string headers = request.substr(0, headers_end);
    std::string lower_headers = to_lower_copy(headers);
    std::string header_name = "content-length:";
    size_t pos = lower_headers.find(header_name);
    if (pos == std::string::npos)
        return 0;

    pos += header_name.size();
    while (pos < headers.size() && (headers[pos] == ' ' || headers[pos] == '\t'))
        ++pos;

    size_t end = pos;
    while (end < headers.size() && std::isdigit(headers[end]))
        ++end;

    if (end == pos)
        return 0;

    std::istringstream iss(headers.substr(pos, end - pos));
    size_t content_length = 0;
    iss >> content_length;
    return content_length;
}

static bool request_is_complete(const std::string &request)
{
    size_t headers_end = find_headers_end(request);
    if (headers_end == std::string::npos)
        return false;

    size_t content_length = extract_content_length(request);
    if (content_length == 0)
        return true;

    return request.size() >= headers_end + content_length;
}

void Server::handleClientRead(int fd)
{
    char buffer[1024];

    int bytes = recv(fd, buffer, 1023, 0);

    if (bytes < 0)
    {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            return;
       _pending_remove.push_back(fd);
        return;
    }

    if (bytes == 0)
    {
        removeClient(fd);
        return;
    }

    std::map<int, Client>::iterator it = _clients.find(fd);

    if (it == _clients.end())
        return;

    Client &c = it->second;

    c.readBuffer.append(buffer, bytes);

    c.request._req = c.readBuffer;
    c.request._client_fd = fd;

    if (!request_is_complete(c.readBuffer))
        return;

    if (!check_response(*this, c.request))
    {
        _pending_remove.push_back(fd);
        return;
    }

    if (!parse_request_line(c.request))
    {
        send_error_page(fd, 400, "Bad Request", "Malformed request line.", c.request._request_id);
        _pending_remove.push_back(fd);
        return;
    }

    if (is_cgi_path(c.request._path))
    {
        if (!handle_cgi_request(*this, c.request))
            send_error_page(fd, 500, "Internal Server Error", "CGI handler failed.", c.request._request_id);
        _pending_remove.push_back(fd);
        return;
    }

    if (c.request._method == "POST")
    {
        this->handle_post_upload(fd, c.request._path, c.request._request_id, c.request._req, c.request._www_root);
        _pending_remove.push_back(fd);
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

        _pending_remove.push_back(fd);
        return;
    }

    if (c.request._method != "GET" && c.request._method != "HEAD")
    {
        send_error_page(fd, 405, "Method Not Allowed", "Only GET, POST and DELETE supported.", c.request._request_id);
        _pending_remove.push_back(fd);
        return;
    }

    if (c.request._path == "/uploads")
    {
        this->handle_uploads_listing(fd, c.request._www_root);
        _pending_remove.push_back(fd);
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
        _pending_remove.push_back(fd);
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
    _pending_remove.push_back(fd);
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
        _pending_remove.push_back(fd);
        return;
    }

    _pending_remove.push_back(fd);
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
