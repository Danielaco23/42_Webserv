#include "Server.hpp"
#include <sstream>
#include <unistd.h>

// Declaración de la función
std::string build_uploads_json(const std::string &uploads_dir);

/**
 * @brief Sends a JSON listing of uploads to the client.
 * @param client_fd Socket file descriptor for the client connection.
 * @param www_root Root directory where uploads are stored.
 */
void Server::handle_uploads_listing(int client_fd,
                                    const std::string &www_root)
{
	std::map<int, Client>::iterator it = _clients.find(client_fd);
	if (it == _clients.end())
		return;

	std::string uploads_dir;
	if (www_root.empty())
		uploads_dir = "www/uploads";
	else
		uploads_dir = www_root + "/uploads";

	std::string body = build_uploads_json(uploads_dir);
	std::ostringstream headers;

	headers << "HTTP/1.1 200 OK\r\n"
			<< "Content-Type: application/json; charset=UTF-8\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "Connection: close\r\n\r\n";

	it->second.writeBuffer = headers.str() + body;
	it->second.state = WRITING;
	enableWriteEvent(client_fd);
}