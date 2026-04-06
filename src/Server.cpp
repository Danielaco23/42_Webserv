#include "../includes/Server.hpp"
#include <limits.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>

static int hex_to_int(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static std::string url_decode(const std::string &input)
{
	std::string out;
	out.reserve(input.size());
	for (size_t i = 0; i < input.size(); ++i)
	{
		if (input[i] == '%' && i + 2 < input.size())
		{
			int hi = hex_to_int(input[i + 1]);
			int lo = hex_to_int(input[i + 2]);
			if (hi >= 0 && lo >= 0)
			{
				out.push_back(static_cast<char>((hi << 4) | lo));
				i += 2;
				continue;
			}
		}
		out.push_back(input[i]);
	}
	return out;
}

static std::string build_request_id()
{
	static unsigned long req_counter = 0;
	++req_counter;
	std::ostringstream reqid_ss;
	reqid_ss << "req-" << req_counter;
	return reqid_ss.str();
}

bool parse_request_line(HttpRequest &parsed_request)
{
	// Parse only the first HTTP line: METHOD SP PATH SP VERSION CRLF
	std::istringstream reqstream(parsed_request._req);
	std::string request_line;

	if (!std::getline(reqstream, request_line))
		return false;
	if (!request_line.empty() && request_line[request_line.size() - 1] == '\r')
		request_line.erase(request_line.size() - 1);

	std::istringstream line_stream(request_line);
	if (!(line_stream >> parsed_request._method >> parsed_request._path >> parsed_request._version))
		return false;

	// Reject malformed request lines with extra tokens.
	std::string extra_token;
	if (line_stream >> extra_token)
		return false;

	size_t query_pos = parsed_request._path.find('?');
	if (query_pos != std::string::npos)
		parsed_request._path = parsed_request._path.substr(0, query_pos);

	parsed_request._path = url_decode(parsed_request._path);

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
        this->_request_data._www_root = std::string(d) + "/www";
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

void Server::initVariables()
{
	this->_number_of_clients = 0;
	this->_request_data._method = "";
	this->_request_data._path = "";
	this->_request_data._version = "";
	this->_request_data._maxBodySize = 0;
	this->_request_data._file_path = "";
	this->_request_data._req = "";
	this->_request_data._request_id = "";
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
	// std::string file_path;
	// std::string req;
    socklen_t addrlen = sizeof(client_addr);
    this->_request_data._client_fd = accept(_server_fd, (struct sockaddr *)&client_addr, &addrlen);

    if (this->_request_data._client_fd < 0)
    {
        perror("accept failed");
        return;
    }

	this->_request_data._request_id = build_request_id();
	handleRequest();
}

int Server::getServerFd() const
{
    return _server_fd;
}