#include "../includes/Server.hpp"
#include <limits.h>
#include <libgen.h>
#include <unistd.h>

Server::Server(int port) : _port(port)
{
    _server_fd = -1;
    std::memset(&_address, 0, sizeof(_address));

    // compute executable directory and set _www_root to <exe_dir>/www
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
    if (len != -1)
    {
        exe_path[len] = '\0';
        char *dirc = strdup(exe_path);
        char *d = dirname(dirc);
        _www_root = std::string(d) + "/www";
        free(dirc);
    }
}

Server::~Server()
{
    if (_server_fd != 1)
        close(_server_fd);
}

void Server::initSocket()
{
	// Creacion Socket
	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_server_fd < 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	// 2. Reuse  port (very important)
	int opt = 1;
	if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt failed");
		exit (EXIT_FAILURE);
	}

	// 3. Configuracion direccion
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr =  INADDR_ANY; // acepta cualquier IP
	_address.sin_port = htons(_port);

	// 4. BIND
	if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
}

void Server::startListening()
{
	if (listen(_server_fd, 10) < 0)
	{
		perror("listen failed");
		exit(EXIT_FAILURE);
	}
	std::cout << "Server listening on port " << _port << std::endl;
    std::cout << "Try accessing http://localhost:" << _port << " in your browser!" << std::endl;
}

void Server::acceptConnection()
{
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int client_fd = accept(_server_fd, (struct sockaddr *)&client_addr, &addrlen);

    if (client_fd < 0)
    {
        perror("accept failed");
        return;
    }

    // generate a small request id for tracing
    static unsigned long req_counter = 0;
    ++req_counter;
    std::ostringstream reqid_ss;
    reqid_ss << "req-" << req_counter;
    std::string request_id = reqid_ss.str();

    std::cout << "New client connected! (" << request_id << ")" << std::endl;

    // 🔹 Leer request
    char buffer[1024];
    int bytes_read = recv(client_fd, buffer, 1023, 0);

    if (bytes_read < 0)
    {
        perror("recv failed");
        close(client_fd);
        return;
    }

    if (bytes_read == 0)
    {
        std::cerr << "Client closed connection before sending data (" << request_id << ")" << std::endl;
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    std::cout << "Request (" << request_id << "):\n" << buffer << std::endl;

	std::string req(buffer);
	std::istringstream reqstream(req);
	std::string method, path, version;
	reqstream >> method >> path >> version;

	if (method != "GET")
	{
		std::cerr << "Unsupported method: " << method << " (" << request_id << ")" << std::endl;
		send_error_page(client_fd, 405, "Method Not Allowed", "Only GET is supported.", request_id);
		return;
	}

    if (path == "/")
		path = "/index.html";

    // prevent path traversal
    if (path.find("..") != std::string::npos)
    {
        send_error_page(client_fd, 400, "Bad Request", "Invalid path.", request_id);
        return;
    }

    // translate URL path to file under the computed www root
    std::string file_path = _www_root.empty() ? (std::string("www") + path) : (_www_root + path);

    std::cout << "Routing " << path << " -> " << file_path << " (" << request_id << ")" << std::endl;

    // attempt to send file; on failure, send 404 using template
    send_file(client_fd, file_path, request_id);
    std::cout << "Finished " << request_id << std::endl;
}

int Server::getServerFd() const
{
    return _server_fd;
}