#include "../includes/Server.hpp"

Server::Server(int port) : _port(port)
{
    _server_fd = -1;
    std::memset(&_address, 0, sizeof(_address));
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
}

void Server::acceptConnection()
{
    int addrlen = sizeof(_address);
    int client_fd = accept(_server_fd, (struct sockaddr *)&_address, (socklen_t *)&addrlen);

    if (client_fd < 0)
    {
        perror("accept failed");
        return;
    }

    std::cout << "New client connected!" << std::endl;

    // 🔹 Leer request
    char buffer[1024];
    int bytes_read = recv(client_fd, buffer, 1023, 0);

    if (bytes_read < 0)
    {
        perror("recv failed");
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    std::cout << "Request:\n" << buffer << std::endl;

    // 🔥 RESPUESTA HTTP (lo importante)
    // std::string response =
    //     "HTTP/1.1 200 OK\r\n"
    //     "Content-Type: text/plain\r\n"
    //     "Content-Length: 5\r\n"
    //     "\r\n"
    //     "Hello";

    // send(client_fd, response.c_str(), response.size(), 0);

	sendWebPage(client_fd);
}

int Server::getServerFd() const
{
    return _server_fd;
}