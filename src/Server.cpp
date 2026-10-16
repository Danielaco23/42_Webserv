#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include <csignal>
#include <cerrno>
#include <vector>

volatile sig_atomic_t g_running = 1;

// ============================
// CONSTRUCTOR / DESTRUCTOR
// ============================

Server::Server(const std::vector<Config> &configs)
{
    
    for (size_t i = 0; i < configs.size(); i++)
    {
        ListeningSocket ls;
        ls.config = configs[i];
        ls.fd = -1;
        _servers.push_back(ls);
    }
}

std::map<int, Client> &Server::getClients()
{
	    return _clients;
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
    for (size_t i = 0; i < _servers.size(); i++)
    {
        if (_servers[i].fd >= 0)
        {
            close(_servers[i].fd);
            _servers[i].fd = -1;
        }
    }

    for (size_t i = 0; i < _clients.size(); ++i)
        ; // los clientes se cierran en removeClient()

    _fds.clear();
    _clients.clear();
}

// ============================
// Auxiliary Functions
// ============================


static std::string getHostString( Config &cfg)
{
    int *host = const_cast<Config &>(cfg).get_host();

    std::ostringstream oss;
    oss << host[0] << "."
        << host[1] << "."
        << host[2] << "."
        << host[3];

    return oss.str();
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

        Config &cfg = _servers[i].config;

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = htons(cfg.get_port());

        std::string ip = getHostString(cfg);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0)
        {
            if (errno == EADDRINUSE)
            {
                std::cerr
                << "Webserv: cannot bind to "
                << getHostString(cfg)
                << ":"
                << cfg.get_port()
                << " because the address is already in use."
                << std::endl;
            }
            else
                perror("bind");

            shutdownServer();
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
       Config &cfg = _servers[i].config;

    std::cout
            << "Server "
            << i
            << " running on http://"
            << getHostString(cfg)
            << ":"
            << cfg.get_port()
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

        if (poll(&_fds[0], _fds.size(), 1000) < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        processPollEvents();
        checkClientTimeouts();
        applyPendingChanges();
    }

    shutdownServer();
    std::cout << "Server stopped cleanly." << std::endl;
}


void Server::processPollEvents()
{
    for (size_t i = 0; i < _fds.size(); i++)
    {
        int fd = _fds[i].fd;
        short events = _fds[i].revents;

        if (events == 0)
            continue;

        if (events & (POLLNVAL | POLLERR))
        {
            _pending_remove.push_back(fd);
            continue;
        }

        if (isCgiPipe(fd))
        {
            if (events & (POLLIN | POLLHUP))
                handleCgiRead(fd);
            continue;
        }

        if (isCgiInputPipe(fd))
        {
            if (events & POLLOUT)
                handleCgiWrite(fd);
            continue;
        }
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
        if (events & POLLHUP)
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
    {
        int fd = _pending_remove[i];

        if (isCgiPipe(fd))
            removeCgiPipe(fd);
        else if (isCgiInputPipe(fd))
            removeCgiInputPipe(fd);
        else
            removeClient(fd);
    }

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

static bool extract_content_length(const std::string &request, size_t &content_length)
{
    size_t headers_end;
    std::string headers;
    std::string lower_headers;
    std::string header_name;
    size_t pos;
    size_t end;
    std::istringstream iss;

    headers_end = find_headers_end(request);

    if (headers_end == std::string::npos)
        return false;

    headers = request.substr(0, headers_end);
    lower_headers = to_lower_copy(headers);
    header_name = "content-length:";

    pos = lower_headers.find(header_name);

    if (pos == std::string::npos)
        return false;

    pos += header_name.size();

    while (pos < headers.size()
        && (headers[pos] == ' ' || headers[pos] == '\t'))
        ++pos;

    end = pos;

    while (end < headers.size()
        && std::isdigit(
            static_cast<unsigned char>(headers[end])))
        ++end;

    if (end == pos)
        return false;

    iss.str(headers.substr(pos, end - pos));
    iss >> content_length;

    return true;
}

static bool has_chunked_encoding(const std::string &request)
{
    size_t headers_end;
    std::string headers;
    std::string lower_headers;

    headers_end = find_headers_end(request);

    if (headers_end == std::string::npos)
        return false;

    headers = request.substr(0, headers_end);
    lower_headers = to_lower_copy(headers);

    return lower_headers.find(
        "transfer-encoding: chunked"
    ) != std::string::npos;
}

static bool parse_chunk_trailers(Client &client)
{
    if (client.readBuffer.size()
        < client.chunkedPos + 2)
        return false;

    if (client.readBuffer.compare(
            client.chunkedPos, 2, "\r\n") == 0)
    {
        client.chunkedPos += 2;
        client.chunkedFinished = true;
        client.chunkedState = CHUNK_DONE;
        return true;
    }

    return false;
}

static bool parse_chunk_crlf(Client &client)
{
    if (client.readBuffer.size()
        < client.chunkedPos + 2)
        return false;

    if (client.readBuffer.compare(
            client.chunkedPos, 2, "\r\n") != 0)
        return false;

    client.chunkedPos += 2;
    client.chunkedSize = 0;
    client.chunkedReceived = 0;
    client.chunkedState = CHUNK_SIZE;

    return true;
}

static bool parse_chunk_data(Client &client)
{
    size_t available;
    size_t remaining;

    if (client.chunkedPos >= client.readBuffer.size())
        return false;

    available = client.readBuffer.size()
        - client.chunkedPos;

    remaining = client.chunkedSize
        - client.chunkedReceived;

    if (available < remaining)
    {
        client.chunkedReceived += available;
        client.chunkedPos += available;
        return false;
    }

    client.chunkedPos += remaining;
    client.chunkedReceived = client.chunkedSize;
    client.chunkedState = CHUNK_CRLF;

    return true;
}

static bool parse_chunk_size(Client &client)
{
    size_t line_end;
    std::string line;
    std::istringstream iss;

    line_end = client.readBuffer.find("\r\n", client.chunkedPos);

    if (line_end == std::string::npos)
        return false;

    line = client.readBuffer.substr(
        client.chunkedPos,
        line_end - client.chunkedPos);

    iss.str(line);
    iss >> std::hex >> client.chunkedSize;

    if (iss.fail())
        return false;

    client.chunkedPos = line_end + 2;
    client.chunkedReceived = 0;

    if (client.chunkedSize == 0)
        client.chunkedState = CHUNK_TRAILERS;
    else
        client.chunkedState = CHUNK_DATA;

    return true;
}

static bool is_chunked_body_complete(Client &client)
{
    size_t headers_end;

    headers_end = find_headers_end(client.readBuffer);

    if (headers_end == std::string::npos)
        return false;

    if (client.chunkedPos == 0)
        client.chunkedPos = headers_end;

    while (true)
    {
        if (client.chunkedState == CHUNK_SIZE)
        {
            if (!parse_chunk_size(client))
                return false;
        }

        if (client.chunkedState == CHUNK_DATA)
        {
            if (!parse_chunk_data(client))
                return false;
        }

        if (client.chunkedState == CHUNK_CRLF)
        {
            if (!parse_chunk_crlf(client))
                return false;
        }

        if (client.chunkedState == CHUNK_TRAILERS)
        {
            return parse_chunk_trailers(client);
        }

        if (client.chunkedState == CHUNK_DONE)
            return true;
    }
}

static bool request_is_complete(Client &client)
{
    size_t headers_end;
    size_t content_length;

    headers_end = find_headers_end(client.readBuffer);

    if (headers_end == std::string::npos)
        return false;

    if (has_chunked_encoding(client.readBuffer))
        return is_chunked_body_complete(client);

    if (!extract_content_length(
            client.readBuffer, content_length))
        return true;

    return client.readBuffer.size()
        >= headers_end + content_length;
}


void Server::handleClientRead(int fd)
{

    char    buffer[65536];

    int bytes = recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);

    if (bytes < 0)
    {
        //_pending_remove.push_back(fd);
        return;
    }

    if (bytes == 0)
    {
        _pending_remove.push_back(fd);
        return;
    }

    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    Client &c = it->second;

    c.last_activity = std::time(NULL);

    c.readBuffer.append(buffer, bytes);
    c.request._client_fd = fd;

    if (!request_is_complete(c))
    {
        return;
    }

    c.request._req = c.readBuffer;

    if (!check_response(*this, c.request))
    {
        //_pending_remove.push_back(fd);
        return;
    }

    if (!parse_request_line(c.request))
    {
        send_error_page(fd, 400, "Bad Request",
            "Malformed request line.",
            c.request._request_id);
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
        //_pending_remove.push_back(fd);
        return;
    }

    Config &cfg = _servers[idx].config;

    const std::string root = cfg.get_root();
    const std::string index = cfg.get_index();

    const std::vector<Location> locations = cfg.get_locations();

    size_t i = 0;
    size_t best_index = 0;
    size_t best_length = 0;
    bool found = false;

    for (i = 0; i < locations.size(); i++)
    {
        std::string location_path = locations[i].get_path();

        if (location_path.empty())
            continue;

        if (c.request._path.find(location_path) != 0)
            continue;

        if (location_path[location_path.size() - 1] != '/'
            && c.request._path.size() > location_path.size()
            && c.request._path[location_path.size()] != '/')
            continue;

        if (location_path.size() > best_length)
        {
            best_index = i;
            best_length = location_path.size();
            found = true;
        }
    }

    if (!found)
    {
        send_error_page(fd, 404, "Not Found",
            "No matching location found.",
            c.request._request_id);
        return;
    }

   const Location &curr_location = locations[best_index];
    std::string location_path = curr_location.get_path();
    std::string location_root = curr_location.get_root();
    std::string location_index = curr_location.get_index();

    if (location_root.empty())
        location_root = root;

    if (location_index.empty())
        location_index = index;

    //DEBUG: comprobar la ubicacion seleccionada
    std::cout << "\n========== DEBUG LOCATION ==========" << std::endl;
    std::cout << "Request path: " << c.request._path << std::endl;
    std::cout << "Selected location: " << curr_location.get_path() << std::endl;
    std::cout << "Location root: " << location_root << std::endl; std::cout << "Location index: " <<
        location_index << std::endl; std::cout << "====================================" << std::endl;

    if (is_cgi_path(c.request._path, curr_location))
    {
        if (!handle_cgi_request(*this, c.request, location_root, curr_location))
            send_error_page(fd, 500, "Internal Server Error",
                "CGI handler failed.",
                c.request._request_id);

        //_pending_remove.push_back(fd);
        return;
    }
    if (c.request._method == "POST")
    {
        if (!curr_location.get_method(LOC_POST_INDEX))
        {
            send_error_page(fd, 405, "Method Not Allowed",
                "POST method is not allowed for this location.",
                c.request._request_id);
            return;
        }

        handle_post_upload(
        fd,
        c.request._path,
        c.request._request_id,
        c.request._req,
        root,
        cfg.get_client_max_body_size()
        );

        //_pending_remove.push_back(fd);
        return;
    }

    if (c.request._method == "DELETE")
    {
        if (!curr_location.get_method(LOC_DELETE_INDEX))
        {
            send_error_page(fd, 405, "Method Not Allowed",
                "DELETE method is not allowed for this location.",
                c.request._request_id);

            //_pending_remove.push_back(fd);
            return;
        }
        std::string fullPath = root + c.request._path;

        if (std::remove(fullPath.c_str()) == 0)
            send_error_page(fd, 204, "No Content", "", c.request._request_id);
        else
            send_error_page(fd, 404, "Not Found", "File not found.", c.request._request_id);

        //_pending_remove.push_back(fd);
        return;
    }

    if (c.request._method != "GET" && c.request._method != "HEAD")
    {
        send_error_page(fd, 405, "Method Not Allowed",
            "Only GET, POST and DELETE supported.",
            c.request._request_id);

        //_pending_remove.push_back(fd);
        return;
    }

    if (c.request._path == "/uploads")
    {
	    handle_uploads_listing(fd, root);
	    return;
    }

    // =========================
    // PATH NORMALIZATION
    // =========================


    std::string path = c.request._path;

    if (path.find("..") != std::string::npos)
    {
        send_error_page(fd, 400, "Bad Request",
            "Invalid path.",
            c.request._request_id);
        return;
    }

    // =========================
    // BUILD FILE PATH
    // =========================
    std::string relative_path;
    std::string file_path;

    if (location_path == "/")
        relative_path = path;
    else
        relative_path = path.substr(location_path.size());

    file_path = location_root;

    if (!relative_path.empty())
    {
        if (file_path[file_path.size() - 1] != '/'
            && relative_path[0] != '/')
            file_path += "/";

        file_path += relative_path;
    }

    //struct stat st;
/*
    if (stat(file_path.c_str(), &st) == 0
        && S_ISDIR(st.st_mode))
    {
        if (!path.empty() && path[path.size() - 1] != '/')
        {
            std::string response;

            response = "HTTP/1.1 301 Moved Permanently\r\n";
            response += "Location: " + path + "/\r\n";
            response += "Content-Length: 0\r\n";
            response += "Connection: close\r\n\r\n";

            std::map<int, Client>::iterator client_it;
            client_it = _clients.find(fd);

            if (client_it == _clients.end())
                return;

            client_it->second.writeBuffer = response;
            client_it->second.state = WRITING;
            enableWriteEvent(fd);
            return;
        }
    }*/

    c.request._file_path = file_path;

    std::cout << "DEBUG file_path: "
            << file_path << std::endl;

    send_file(fd, c.request._file_path,
        c.request._request_id, cfg);

    return;

}   
// ============================
// WRITE CLIENT
// ============================

void Server::enableWriteEvent(int fd)
{
	for (size_t i = 0; i < _fds.size(); i++)
	{
		if (_fds[i].fd == fd)
		{
			_fds[i].events |= POLLOUT;
			return;
		}
	}
}


// ============================
// write client
// ============================

void Server::handleClientWrite(int fd)
{
    std::map<int, Client>::iterator it;

    it = _clients.find(fd);
    if (it == _clients.end())
        return;

    Client &c = it->second;

    std::cout << "handleClientWrite fd=" << fd
              << " buffer size=" << c.writeBuffer.size()
              << std::endl;

    if (c.writeBuffer.empty())
    {
        _pending_remove.push_back(fd);
        return;
    }

    ssize_t sent;

    sent = send(fd, c.writeBuffer.c_str(), c.writeBuffer.size(), 0);

    std::cout << "send() devolvio: " << sent
              << " bytes" << std::endl;

    if (sent < 0)
    {
        _pending_remove.push_back(fd);
        return;
    }

    if (sent == 0)
    {
        _pending_remove.push_back(fd);
        return;
    }

    c.writeBuffer.erase(0, static_cast<size_t>(sent));

    std::cout << "Quedan: "
              << c.writeBuffer.size()
              << " bytes" << std::endl;

    if (c.writeBuffer.empty())
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


// ============================
// Clear inactive clients after timeout
// ============================


void Server::checkClientTimeouts()
{
    time_t now = std::time(NULL);

    for (std::map<int, Client>::iterator it = _clients.begin();
         it != _clients.end();
         ++it)
    {
        Client &c = it->second;

        if (now - c.last_activity > 30)
        {
            std::cout << "Client " << c.fd << " inactive for " << now - c.last_activity << " seconds" << std::endl;

            _pending_remove.push_back(c.fd);
        }
    }
}


bool Server::isCgiPipe(int fd)
{
    // Check if the file descriptor is in the _cgi_pipes map
    return _cgi_pipes.find(fd) != _cgi_pipes.end();
}

void Server::addCgiPipe(int pipe_fd, int client_fd)
{
    pollfd pfd;

    pfd.fd = pipe_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    _cgi_pipes[pipe_fd] = client_fd;
    _fds.push_back(pfd);
}

void Server::addCgiInputPipe(int pipe_fd, int client_fd)
{
    pollfd pfd;

    pfd.fd = pipe_fd;
    pfd.events = POLLOUT;
    pfd.revents = 0;

    _cgi_input_pipes[pipe_fd] = client_fd;
    _fds.push_back(pfd);
}


void Server::addCgiPid(int pipe_fd, pid_t pid)
{
    _cgi_pids[pipe_fd] = pid;
}

int Server::getCgiClient(int fd)
{
    std::map<int, int>::iterator it = _cgi_pipes.find(fd);
    if (it == _cgi_pipes.end())
        return -1;
    return it->second;
}

void Server::removeCgiPipe(int fd)
{
    _cgi_pipes.erase(fd);
    _cgi_pids.erase(fd);
    close(fd);

    for (size_t i = 0; i < _fds.size(); i++)
    {
        if (_fds[i].fd == fd)
        {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }
}

void Server::removeCgiInputPipe(int fd)
{
    _cgi_input_pipes.erase(fd);
    close(fd);

    for (size_t i = 0; i < _fds.size(); i++)
    {
        if (_fds[i].fd == fd)
        {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }
}


bool Server::isCgiInputPipe(int fd)
{
    return _cgi_input_pipes.find(fd) != _cgi_input_pipes.end();
}

void Server::handleCgiWrite(int fd)
{
    std::map<int, int>::iterator pipe_it;
    std::map<int, Client>::iterator client_it;

    pipe_it = _cgi_input_pipes.find(fd);
    if (pipe_it == _cgi_input_pipes.end())
        return;

    client_it = _clients.find(pipe_it->second);

    if (client_it == _clients.end())
    {
        removeCgiInputPipe(fd);
        return;
    }

    Client &client = client_it->second;

    if (client.cgiInputPos >= client.cgiInput.size())
    {
        removeCgiInputPipe(fd);
        return;
    }

    ssize_t sent;

    sent = write(
        fd,
        client.cgiInput.c_str() + client.cgiInputPos,
        client.cgiInput.size() - client.cgiInputPos
    );

    if (sent > 0)
    {
        client.cgiInputPos += static_cast<size_t>(sent);

        if (client.cgiInputPos >= client.cgiInput.size())
            removeCgiInputPipe(fd);
    }
    else if (sent == 0)
    {
        removeCgiInputPipe(fd);
    }
}