#include "Server.hpp"

static const size_t kMaxUploadBodyBytes = 1000000;

static void respond_upload_success(int client_fd)
{
	std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 16\r\nConnection: close\r\n\r\nFiles uploaded.\n";
	send(client_fd, response.c_str(), response.size(), 0);
	close(client_fd);
}

static std::string extract_boundary(const std::string &headers_part)
{
	size_t ct_pos = headers_part.find("Content-Type: multipart/form-data");
	if (ct_pos == std::string::npos)
		return "";

	size_t bound_pos = headers_part.find("boundary=", ct_pos);
	if (bound_pos == std::string::npos)
		return "";

	bound_pos += 9;
	size_t bound_end = headers_part.find("\r\n", bound_pos);
	if (bound_end == std::string::npos)
		bound_end = headers_part.find("\n", bound_pos);
	return "--" + headers_part.substr(bound_pos, bound_end - bound_pos);
}

static size_t extract_content_length(const std::string &headers_part)
{
	size_t clen_pos = headers_part.find("Content-Length:");
	if (clen_pos == std::string::npos)
		return 0;

	size_t start = clen_pos + 15;
	size_t end = headers_part.find("\r\n", start);
	if (end == std::string::npos)
		end = headers_part.find("\n", start);
	std::string len_str = headers_part.substr(start, end - start);
	std::istringstream iss(len_str);
	size_t value = 0;
	iss >> value;
	return value;
}

static bool split_headers_and_body(const std::string &request, std::string &headers, std::string &body)
{
	size_t pos = request.find("\r\n\r\n");
	size_t sep_len = 4;
	if (pos == std::string::npos)
	{
		pos = request.find("\n\n");
		sep_len = 2;
	}
	if (pos == std::string::npos)
		return false;

	headers = request.substr(0, pos + sep_len);
	body = request.substr(pos + sep_len);
	return true;
}

static int save_multipart_files(const std::string &body, const std::string &boundary, const std::string &www_root)
{
	int saved_files = 0;
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
		{
			if (save_uploaded_file(www_root, filename, content))
				++saved_files;
		}

		part_start = part_end;
	}
	return saved_files;
}

void handle_post_upload(int client_fd, const std::string &path, const std::string &request_id, const std::string &request, const std::string &www_root)
{
	std::cout << "POST " << path << " (" << request_id << ")" << std::endl;

	std::string headers_part;
	std::string buffered_body;
	if (!split_headers_and_body(request, headers_part, buffered_body))
	{
		std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 16\r\nConnection: close\r\n\r\nMalformed request";
		send(client_fd, response.c_str(), response.size(), 0);
		close(client_fd);
		return;
	}

	size_t content_length = extract_content_length(headers_part);
	if (content_length == 0 && headers_part.find("Content-Length:") == std::string::npos)
	{
		std::string response = "HTTP/1.1 411 Length Required\r\nContent-Type: text/plain\r\nContent-Length: 29\r\nConnection: close\r\n\r\nContent-Length header required";
		send(client_fd, response.c_str(), response.size(), 0);
		close(client_fd);
		return;
	}
	if (content_length > kMaxUploadBodyBytes)
	{
		std::string response = "HTTP/1.1 413 Payload Too Large\r\nContent-Type: text/plain\r\nContent-Length: 22\r\nConnection: close\r\n\r\nPayload too large.\n";
		send(client_fd, response.c_str(), response.size(), 0);
		close(client_fd);
		return;
	}

	std::string body = buffered_body;
	if (body.size() < content_length)
		body += read_request_body(client_fd, content_length - body.size());
	if (body.size() > content_length)
		body.erase(content_length);
	std::string boundary = extract_boundary(headers_part);

	if (boundary.empty())
	{
		std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 29\r\nConnection: close\r\n\r\nInvalid multipart form request.";
		send(client_fd, response.c_str(), response.size(), 0);
		close(client_fd);
		return;
	}

	int saved_files = save_multipart_files(body, boundary, www_root);
	if (saved_files <= 0)
	{
		std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 29\r\nConnection: close\r\n\r\nNo valid files found in upload.";
		send(client_fd, response.c_str(), response.size(), 0);
		close(client_fd);
		return;
	}

	respond_upload_success(client_fd);
}