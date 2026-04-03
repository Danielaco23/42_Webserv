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
    std::string normalized_path = (request_path == "/") ? "/index.html" : request_path;
    if (normalized_path.find("..") != std::string::npos)
        return false;

    file_path = www_root.empty() ? (std::string("www") + normalized_path) : (www_root + normalized_path);
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
    socklen_t addrlen = sizeof(client_addr);
    int client_fd = accept(_server_fd, (struct sockaddr *)&client_addr, &addrlen);

    if (client_fd < 0)
    {
        perror("accept failed");
        return;
    }

    static unsigned long req_counter = 0;
    _number_of_clients = 0;
    ++req_counter;
    std::ostringstream reqid_ss;
    reqid_ss << "req-" << req_counter;
    std::string request_id = reqid_ss.str();

    // std::cout << "New client connected! (" << request_id << ")" << std::endl;

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
    // std::cout << "Request (" << request_id << "):\n" << buffer << std::endl;

	std::string req(buffer);
	std::istringstream reqstream(req);
	std::string method, path, version;
    if (!(reqstream >> method >> path >> version))
    {
        send_error_page(client_fd, 400, "Bad Request", "Malformed request line.", request_id);
        return;
    }

	if (method == "POST")
	{
		std::cout << "POST " << path << " (" << request_id << ")" << std::endl;
		
		// Extract Content-Length from headers
		std::string headers_part(buffer);
		size_t clen_pos = headers_part.find("Content-Length:");
		size_t content_length = 0;
		if (clen_pos != std::string::npos)
		{
			size_t start = clen_pos + 15;
			size_t end = headers_part.find("\r\n", start);
			if (end == std::string::npos)
				end = headers_part.find("\n", start);
			std::string len_str = headers_part.substr(start, end - start);
			content_length = std::stoul(len_str);
		}
		
		// Read the full body
		std::string body = read_request_body(client_fd, content_length);
		
		// Find boundary from Content-Type header
		size_t ct_pos = headers_part.find("Content-Type: multipart/form-data");
		std::string boundary;
		if (ct_pos != std::string::npos)
		{
			size_t bound_pos = headers_part.find("boundary=", ct_pos);
			if (bound_pos != std::string::npos)
			{
				bound_pos += 9;
				size_t bound_end = headers_part.find("\r\n", bound_pos);
				if (bound_end == std::string::npos)
					bound_end = headers_part.find("\n", bound_pos);
				boundary = "--" + headers_part.substr(bound_pos, bound_end - bound_pos);
			}
		}
		
		// Extract and save files
		if (!boundary.empty())
		{
			size_t part_start = body.find(boundary);
			while (part_start != std::string::npos)
			{
				part_start += boundary.size();
				size_t part_end = body.find(boundary, part_start);
				if (part_end == std::string::npos)
					break;
				
				std::string part = body.substr(part_start, part_end - part_start);
				std::string filename, content;
				if (extract_multipart_file(part, filename, content))
					save_uploaded_file(_www_root, filename, content);
				
				part_start = part_end;
			}
		}
		
		std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 16\r\nConnection: close\r\n\r\nFiles uploaded.\n";
		send(client_fd, response.c_str(), response.size(), 0);
		close(client_fd);
		return;
	}
	else if (method != "GET")
	{
		std::cerr << "Unsupported method: " << method << " (" << request_id << ")" << std::endl;
		send_error_page(client_fd, 405, "Method Not Allowed", "Only GET and POST are supported.", request_id);
		return;
	}

    std::string file_path;
    if (!resolve_requested_path(path, _www_root, file_path))
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

    std::cout << "The web asks for " << path << " in reality, it is -> " << file_path << " (" << request_id << ")" << std::endl;


    // Serve file (send_file handles 404 fallback when file cannot be opened).
    send_file(client_fd, file_path, request_id);

    // std::cout << "Finished " << request_id << std::endl;
}

int Server::getServerFd() const
{
    return _server_fd;
}