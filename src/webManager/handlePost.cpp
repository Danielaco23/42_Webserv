#include "Server.hpp"

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
	return std::stoul(len_str);
}

static void save_multipart_files(const std::string &body, const std::string &boundary, const std::string &www_root)
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
			save_uploaded_file(www_root, filename, content);

		part_start = part_end;
	}
}

void handle_post_upload(int client_fd, const std::string &path, const std::string &request_id, const std::string &request, const std::string &www_root)
{
	std::cout << "POST " << path << " (" << request_id << ")" << std::endl;

	std::string headers_part(request);
	size_t content_length = extract_content_length(headers_part);
	std::string body = read_request_body(client_fd, content_length);
	std::string boundary = extract_boundary(headers_part);

	if (!boundary.empty())
		save_multipart_files(body, boundary, www_root);

	respond_upload_success(client_fd);
}