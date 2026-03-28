#include "../includes/Server.hpp"

static bool sendAll(int fd, const char *buf, size_t len)
{
	size_t sent = 0;
	while (sent < len)
	{
		ssize_t n = send(fd, buf + sent, static_cast<size_t>(len - sent), 0);
		if (n < 0)
		{
			if (errno == EINTR) continue;
			return false;
		}
		if (n == 0) return false;
		sent += static_cast<size_t>(n);
	}
	return true;
}

static bool sendAll(int fd, const std::string &s)
{
	return sendAll(fd, s.data(), s.size());
}

// send an HTML file as an HTTP response
void Server::sendWebPage(int client_fd)
{
	const char *path = "www/index.html";

	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file)
	{
		const char *notfound =
			"HTTP/1.1 404 Not Found\r\n"
			"Content-Type: text/plain; charset=UTF-8\r\n"
			"Content-Length: 9\r\n"
			"Connection: close\r\n"
			"\r\n"
			"Not found";
		sendAll(client_fd, notfound, strlen(notfound));
		close(client_fd);
		return;
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	std::string body = buffer.str();

	std::ostringstream lenoss;
	lenoss << body.size();
	std::string len = lenoss.str();

	std::string headers;
	headers  = "HTTP/1.1 200 OK\r\n";
	headers += "Content-Type: text/html; charset=UTF-8\r\n";
	headers += "Content-Length: ";
	headers += len;
	headers += "\r\n";
	headers += "Connection: close\r\n";
	headers += "\r\n";

	sendAll(client_fd, headers);
	sendAll(client_fd, body);

	close(client_fd);
}