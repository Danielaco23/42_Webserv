#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include <csignal>
#include <cerrno>

volatile sig_atomic_t g_running = 1;

// ============================
// CONSTRUCTOR / DESTRUCTOR
// ============================

Server::Server(const std::vector<ServerConfig> &configs)
{
    for (size_t i = 0; i < configs.size(); i++)
    {
        ListeningSocket ls;
        ls.config = configs[i];
        ls.fd = -1;
        _servers.push_back(ls);
    }
}

Server::~Server()
{
    //  Por los signals 
   // if (_server_fd != -1)
   //     close(_server_fd);
}

// ============================
// SIGNALS
// ============================

void signalHandler(int signal)
{
    (void)signal;
    g_running = 0;
}

void Server::shutdownServer()
{
    for (size_t i = 0; i < _fds.size(); i++)
    {
        if (_fds[i].fd >= 0)
            close(_fds[i].fd);
    }

    _fds.clear();
    _clients.clear();

    for (size_t i = 0; i < _servers.size(); i++)
    {
        if (_servers[i].fd != -1)
            close(_servers[i].fd);
    }
}

// ============================
// INIT VARIABLES
// ============================
/*
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
}*/

void Server::initVariables()
{
    _number_of_clients = 0;
}

// ============================
// INIT SOCKET
// ============================

void Server::initSocket()
{
    for (size_t i = 0; i < _servers.size(); i++)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        ServerConfig &cfg = _servers[i].config;

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = htons(cfg.port);
        addr.sin_addr.s_addr = inet_addr(cfg.host.c_str());

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0)
        {
            perror("bind");
            exit(EXIT_FAILURE);
        }

        if (listen(fd, 128) < 0)
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        fcntl(fd, F_SETFL, O_NONBLOCK);

        _servers[i].fd = fd;

        pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        _fds.push_back(pfd);
    }
}

// ============================
// START LISTENING
// ============================

void Server::startListening()
{
    for (size_t i = 0; i < _servers.size(); i++)
    {
        std::cout
            << "Server "
            << i
            << " running on http://"
            << _servers[i].config.host
            << ":"
            << _servers[i].config.port
            << std::endl;
    }
}

// ============================
// RUN LOOP
// ============================

void Server::run()
{
    while (g_running)
    {
        if (_fds.empty())
            continue;

        if (poll(&_fds[0], _fds.size(), -1) < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        processPollEvents();
        applyPendingChanges();
    }

    shutdownServer();
    std::cout << "Server stopped cleanly." << std::endl;
}


void Server::processPollEvents()
{
    for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
    {
        int fd = it->fd;
        short events = it->revents;

        if (events == 0)
            continue;

        // =========================
        // NEW CONNECTION
        // =========================
        if (events & POLLIN)
        {
            if (isListeningSocket(fd))
                acceptClient(fd);
            else
                handleClientRead(fd);
        }

        // =========================
        // WRITE
        // =========================
        if (events & POLLOUT)
            handleClientWrite(fd);

        // =========================
        // ERRORS
        // =========================
        if (events & (POLLERR | POLLHUP | POLLNVAL))
            _pending_remove.push_back(fd);
    }
}
void Server::applyPendingChanges()
{
    // =========================
    // ADD NEW CLIENTS
    // =========================
    for (size_t i = 0; i < _pending_add.size(); i++)
        _fds.push_back(_pending_add[i]);

    _pending_add.clear();

    // =========================
    // REMOVE CLIENTS
    // =========================
    for (size_t i = 0; i < _pending_remove.size(); i++)
        removeClient(_pending_remove[i]);

    _pending_remove.clear();
}

// ============================
// ACCEPT CLIENT
// ============================

void Server::acceptClient(int listening_fd)
{
    int fd = accept(listening_fd, NULL, NULL);
    if (fd < 0)
        return;

    fcntl(fd, F_SETFL, O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    _pending_add.push_back(pfd);

    Client client(fd);

    client.server_index = findServerIndex(listening_fd); // 👈 CLAVE
    if (client.server_index == -1)
    {
        close(fd);
        return;
    }
    _clients.insert(std::make_pair(fd, client));

    std::cout << "Client connected: " << fd
              << " (server index: " << client.server_index << ")"
              << std::endl;
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

    int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

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

    // Esperar request completo
    if (!request_is_complete(c.readBuffer))
        return;

    if (!check_response(*this, c.request))
    {
        _pending_remove.push_back(fd);
        return;
    }

    if (!parse_request_line(c.request))
    {
        send_error_page(fd, 400, "Bad Request",
            "Malformed request line.",
            c.request._request_id);
        _pending_remove.push_back(fd);
        return;
    }

    if (is_cgi_path(c.request._path))
    {
        if (!handle_cgi_request(*this, c.request))
            send_error_page(fd, 500, "Internal Server Error",
                "CGI handler failed.",
                c.request._request_id);

        _pending_remove.push_back(fd);
        return;
    }

    // =========================
    // CONFIG LOCAL (CORE CLEAN)
    // =========================
    int idx = c.server_index;
    if (idx < 0 || idx >= (int)_servers.size())
    {
        send_error_page(fd, 500, "Internal Server Error",
            "Invalid server configuration.",
            c.request._request_id);
        _pending_remove.push_back(fd);
        return;
    }

    const ServerConfig &cfg = _servers[idx].config;

    const std::string &root  = cfg.root;
    const std::string &index = cfg.index;

    if (c.request._method == "POST")
    {
        handle_post_upload(
            fd,
            c.request._path,
            c.request._request_id,
            c.request._req,
            root,
            cfg.client_max_body_size
        );

        _pending_remove.push_back(fd);
        return;
    }

    if (c.request._method == "DELETE")
    {
        std::string fullPath = root + c.request._path;

        if (std::remove(fullPath.c_str()) == 0)
            send_error_page(fd, 204, "No Content", "", c.request._request_id);
        else
            send_error_page(fd, 404, "Not Found", "File not found.", c.request._request_id);

        _pending_remove.push_back(fd);
        return;
    }

    if (c.request._method != "GET" && c.request._method != "HEAD")
    {
        send_error_page(fd, 405, "Method Not Allowed",
            "Only GET, POST and DELETE supported.",
            c.request._request_id);

        _pending_remove.push_back(fd);
        return;
    }

    if (c.request._path == "/uploads")
    {
        handle_uploads_listing(fd, root);
        _pending_remove.push_back(fd);
        return;
    }

    // =========================
    // PATH NORMALIZATION
    // =========================

    std::string path = c.request._path;

    if (path == "/")
        path = "/" + index;

    if (path.find("..") != std::string::npos)
    {
        send_error_page(fd, 400, "Bad Request",
            "Invalid path.",
            c.request._request_id);

        _pending_remove.push_back(fd);
        return;
    }

    // =========================
    // BUILD FILE PATH
    // =========================

    std::string file_path = root + path;

    struct stat st;
    if (stat(file_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        file_path += "/" + index;
    else if (stat(file_path.c_str(), &st) != 0 && path.find('.') == std::string::npos)
        file_path = root + path + "/" + index;

    c.request._file_path = file_path;

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


// ============================
// 
// ============================

bool Server::isListeningSocket(int fd)
{
    for (size_t i = 0; i < _servers.size(); i++)
    {
        if (_servers[i].fd == fd)
            return true;
    }
    return false;
}
// ============================
// FIND SERVER INDEX
// ============================

int Server::findServerIndex(int listening_fd)
{
    for (size_t i = 0; i < _servers.size(); i++)
    {
        if (_servers[i].fd == listening_fd)
            return i;
    }
    return -1;
}


//===========================
// GET CONFIG BY LISTEN FD (multiserver)
//===========================


/*Config &Server::getConfigByListenFd(int fd)
{
    (void)fd;
    return _config;
}*/