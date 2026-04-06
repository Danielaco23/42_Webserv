#include "Server.hpp"
#include <cctype>

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

static size_t extract_content_length(const std::string &request)
{
	size_t headers_end = find_headers_end(request);
	if (headers_end == std::string::npos)
		return 0;

	std::string headers = request.substr(0, headers_end);
	size_t pos = headers.find("Content-Length:");
	if (pos == std::string::npos)
		return 0;

	pos += 15;
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

/**
 * @brief Normalizes the request path and resolves it against the configured document root.
 * @param request_data Request context containing path input and resolved file output.
 * @return true if the path is valid and resolved successfully, false otherwise.
 */
static bool resolve_requested_path(HttpRequest &request_data)
{
	std::string normalized_path;
	if (request_data._path == "/")
		normalized_path = "/index.html";
	else
		normalized_path = request_data._path;
    
	if (normalized_path.find("..") != std::string::npos)
        return (false);

	if (request_data._www_root.empty())
		request_data._file_path = std::string("www") + normalized_path;
	else
		request_data._file_path = request_data._www_root + normalized_path;
    return (true);
}

static bool receive_request(int client_fd, const std::string &request_id, std::string &request)
{
	char buffer[4096];
	request.clear();

	size_t target_size = 0;
	bool have_headers = false;

	while (true)
	{
		int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
		if (bytes_read < 0)
		{
			perror("recv failed");
			close(client_fd);
			return false;
		}

		if (bytes_read == 0)
		{
			if (request.empty())
			{
				std::cerr << "Client closed connection before sending data (" << request_id << ")" << std::endl;
				close(client_fd);
				return false;
			}
			break;
		}

		request.append(buffer, static_cast<size_t>(bytes_read));

		if (!have_headers)
		{
			size_t headers_end = find_headers_end(request);
			if (headers_end == std::string::npos)
				continue;
			have_headers = true;
			target_size = headers_end + extract_content_length(request);
		}

		if (have_headers && request.size() >= target_size)
			break;
	}

	return true;
}

void Server::handleRequest()
{
	if (!receive_request(this->_request_data._client_fd, this->_request_data._request_id, this->_request_data._req))
    {
        send_error_page(this->_request_data._client_fd, 400, "Bad Request", "Failed to receive request data.", this->_request_data._request_id);
        return;
    }
    if (!check_response(*this, this->_request_data))
    {
        send_error_page(this->_request_data._client_fd, 400, "Bad Request", "Malformed request.", this->_request_data._request_id);
        return;
    }

	if (!parse_request_line(this->_request_data))
    {
        send_error_page(this->_request_data._client_fd, 400, "Bad Request", "Malformed request line.", this->_request_data._request_id);
        return ;
    }

	// Delegate only /cgi-bin and /cgi-bin/* to CGI. All other routes use native handlers.
	const bool is_cgi_route = (
		this->_request_data._path == "/cgi-bin"
		|| this->_request_data._path == "/cgi-bin/"
		|| this->_request_data._path.compare(0, 9, "/cgi-bin/") == 0
	);
	if (is_cgi_route)
	{
		if (!handle_cgi_request(*this, this->_request_data))
			send_error_page(this->_request_data._client_fd, 500, "Internal Server Error", "CGI handler failed.", this->_request_data._request_id);
		return;
	}

	if (this->_request_data._method == "POST")
	{
		handle_post_upload(this->_request_data._client_fd, this->_request_data._path, this->_request_data._request_id, this->_request_data._req, this->_request_data._www_root);
		return;
	}
	else if (this->_request_data._method  == "DELETE")
	{
		std::string fullPath;
		if (this->_request_data._www_root.empty())
			fullPath = std::string("www") + this->_request_data._path;
		else
			fullPath = this->_request_data._www_root + this->_request_data._path;

		if (std::remove(fullPath.c_str()) == 0) {
			send_error_page(this->_request_data._client_fd, 204, "No Content", "", this->_request_data._request_id);
			return;
		} else {
			send_error_page(this->_request_data._client_fd, 404, "Not Found", "File not found.", this->_request_data._request_id);
			return;
		}
	}
	else if (this->_request_data._method != "GET" && this->_request_data._method != "HEAD")
	{
		std::cerr << "Unsupported method: " << this->_request_data._method << " (" << this->_request_data._request_id << ")" << std::endl;
		send_error_page(this->_request_data._client_fd, 405, "Method Not Allowed", "Only GET, POST and DELETE are supported.", this->_request_data._request_id);
		return ;
	}
	else if (this->_request_data._path == "/uploads")
	{
		handle_uploads_listing(this->_request_data._client_fd, this->_request_data._www_root);
		return ;
	}
	else if (this->_request_data._path == "/showUploads")
	{
		std::string show_uploads_path;
		if (this->_request_data._www_root.empty())
			show_uploads_path = "www/showUploads/index.html";
		else
			show_uploads_path = this->_request_data._www_root + "/showUploads/index.html";
		send_file(this->_request_data._client_fd, show_uploads_path, this->_request_data._request_id);
		return ;
	}

	if (!resolve_requested_path(this->_request_data))
    {
        send_error_page(this->_request_data._client_fd, 400, "Bad Request", "Invalid path.", this->_request_data._request_id);
        return;
    }

    if (this->_request_data._file_path.find("index.html") != std::string::npos)
    {
        ++_number_of_clients;
        std::cout << GREEN << "Number of clients: " << _number_of_clients << NC << std::endl;
    }

	std::cout << "The web asks for " << this->_request_data._path << " in reality, it is -> " << this->_request_data._file_path << " (" << this->_request_data._request_id << ")" << std::endl;
    send_file(this->_request_data._client_fd, this->_request_data._file_path, this->_request_data._request_id);
}