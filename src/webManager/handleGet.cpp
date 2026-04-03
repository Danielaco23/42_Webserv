#include "Server.hpp"

void handle_uploads_listing(int client_fd, const std::string &www_root)
{
	std::string body = build_uploads_json(www_root);
	std::ostringstream headers;
	headers << "HTTP/1.1 200 OK\r\n"
			<< "Content-Type: application/json; charset=UTF-8\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "Connection: close\r\n\r\n";

	std::string response = headers.str() + body;
	send(client_fd, response.c_str(), response.size(), 0);
	close(client_fd);
}
