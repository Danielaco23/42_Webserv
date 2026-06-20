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

	std::string base_path;
	if (request_data._www_root.empty())
		base_path = "www";
	else
		base_path = request_data._www_root;

	request_data._file_path = base_path + normalized_path;
	struct stat st;
	if (stat(request_data._file_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
		request_data._file_path += "/index.html";
	else if (stat(request_data._file_path.c_str(), &st) != 0 && normalized_path.find('.') == std::string::npos)
		request_data._file_path = base_path + normalized_path + "/index.html";
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
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        HttpRequest &req = it->second.request;

        if (!receive_request(req._client_fd, req._request_id, req._req))
        {
            send_error_page(req._client_fd, 400, "Bad Request",
                "Failed to receive request data.", req._request_id);
            continue;
        }

        if (!check_response(*this, req))
        {
            send_error_page(req._client_fd, 400, "Bad Request",
                "Malformed request.", req._request_id);
            continue;
        }

        if (!parse_request_line(req))
        {
            send_error_page(req._client_fd, 400, "Bad Request",
                "Malformed request line.", req._request_id);
            continue;
        }

        if (is_cgi_path(req._path))
        {
            if (!handle_cgi_request(*this, req))
                send_error_page(req._client_fd, 500,
                    "Internal Server Error",
                    "CGI handler failed.",
                    req._request_id);
            continue;
        }

        if (req._method == "POST")
        {
            this->handle_post_upload(req._client_fd, req._path,
                req._request_id, req._req, req._www_root);
            continue;
        }

        if (req._method == "DELETE")
        {
            std::string fullPath =
                req._www_root.empty()
                    ? std::string("www") + req._path
                    : req._www_root + req._path;

            if (std::remove(fullPath.c_str()) == 0)
                send_error_page(req._client_fd, 204, "No Content", "", req._request_id);
            else
                send_error_page(req._client_fd, 404, "Not Found",
                    "File not found.", req._request_id);

            continue;
        }

        if (req._method != "GET" && req._method != "HEAD")
        {
            send_error_page(req._client_fd, 405,
                "Method Not Allowed",
                "Only GET, POST and DELETE supported.",
                req._request_id);
            continue;
        }

        if (req._path == "/uploads")
        {
            this->handle_uploads_listing(req._client_fd, req._www_root);
            continue;
        }

        if (!resolve_requested_path(req))
        {
            send_error_page(req._client_fd, 400,
                "Bad Request", "Invalid path.", req._request_id);
            continue;
        }

        send_file(req._client_fd, req._file_path, req._request_id);
    }
}