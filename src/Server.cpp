#include "../includes/Server.hpp"
#include <limits.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>

/**
 * @brief Normalizes a request path and resolves it against the document root.
 * @param request_path Path extracted from the HTTP request line.
 * @param www_root Document root used to resolve the file path.
 * @param file_path Output resolved filesystem path.
 * @return true if the path is valid and resolved successfully, false otherwise.
 */
static bool resolve_requested_path(const std::string &request_path, const std::string &www_root, std::string &file_path)
{
	std::string normalized_path;
	if (request_path == "/")
		normalized_path = "/index.html";
	else
		normalized_path = request_path;
    if (normalized_path.find("..") != std::string::npos)
        return false;

	if (www_root.empty())
		file_path = std::string("www") + normalized_path;
	else
		file_path = www_root + normalized_path;
    return true;
}

static std::string build_request_id()
{
	static unsigned long req_counter = 0;
	++req_counter;
	std::ostringstream reqid_ss;
	reqid_ss << "req-" << req_counter;
	return reqid_ss.str();
}

bool parse_request_line(const std::string &request, HttpRequest &parsed_request)
{
	std::istringstream reqstream(request);
	std::string request_line;

	if (!std::getline(reqstream, request_line))
		return false;
	if (!request_line.empty() && request_line[request_line.size() - 1] == '\r')
		request_line.erase(request_line.size() - 1);

	std::istringstream line_stream(request_line);
	if (!(line_stream >> parsed_request.method >> parsed_request.path >> parsed_request.version))
		return false;
	return true;
}

static bool receive_request(int client_fd, const std::string &request_id, std::string &request)
{
	char buffer[1024];
	int bytes_read = recv(client_fd, buffer, 1023, 0);

	if (bytes_read < 0)
	{
		perror("recv failed");
		close(client_fd);
		return false;
	}

	if (bytes_read == 0)
	{
		std::cerr << "Client closed connection before sending data (" << request_id << ")" << std::endl;
		close(client_fd);
		return false;
	}

	buffer[bytes_read] = '\0';
	request.assign(buffer);
	return true;
}

/**
 * @brief Constructs a Server instance.
 * @param port Port number to listen on.
 */
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

/**
 * @brief Initializes the server socket.
    * Creates a TCP socket, sets SO_REUSEADDR, binds to the specified port and any IP.
 */
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
    std::cout << "Try accessing http://localhost:" << _port << " in your browser!" << std::endl << std::endl;
}

void Server::acceptConnection()
{
    struct sockaddr_storage client_addr;
	std::string file_path;
	std::string req;
    socklen_t addrlen = sizeof(client_addr);
    int client_fd = accept(_server_fd, (struct sockaddr *)&client_addr, &addrlen);

    if (client_fd < 0)
    {
        perror("accept failed");
        return;
    }

    _number_of_clients = 0;
	std::string request_id = build_request_id();

	if (!receive_request(client_fd, request_id, req) || !check_response(*this, req, request_id, client_fd))
		return;
	// std::cout << "Request (" << request_id << "):\n" << req << std::endl;

	if (!parse_request_line(req, _request_data))
    {
        send_error_page(client_fd, 400, "Bad Request", "Malformed request line.", request_id);
        return ;
    }

	if (_request_data.method == "POST")
	{
		handle_post_upload(client_fd, _request_data.path, request_id, req, _www_root);
		return;
	}
	else if (_request_data.method != "GET")
	{
		std::cerr << "Unsupported method: " << _request_data.method << " (" << request_id << ")" << std::endl;
		send_error_page(client_fd, 405, "Method Not Allowed", "Only GET and POST are supported.", request_id);
		return ;
	}
	else if (_request_data.path == "/uploads")
	{
		handle_uploads_listing(client_fd, _www_root);
		return ;
	}

	if (!resolve_requested_path(_request_data.path, _www_root, file_path))
    {
        send_error_page(client_fd, 400, "Bad Request", "Invalid path.", request_id);
        return;
    }
    
    // In case, we need to keep track of how many clients we have connected
    // SI accede al index, se suma al contador, si se desconecta, se resta.
    if (file_path.find("index.html") != std::string::npos)
    {
        ++_number_of_clients;
        std::cout << "Number of clients: " << _number_of_clients << std::endl;
    }

	std::cout << "The web asks for " << _request_data.path << " in reality, it is -> " << file_path << " (" << request_id << ")" << std::endl;
    send_file(client_fd, file_path, request_id);

    // std::cout << "Finished " << request_id << std::endl;
}

int Server::getServerFd() const
{
    return _server_fd;
}