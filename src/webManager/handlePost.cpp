#include "../includes/Server.hpp"

// ===== Responses =====
static void respond_upload_success(int client_fd)
{
	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: 16\r\n"
		"Connection: close\r\n\r\n"
		"Files uploaded.\n";

	send(client_fd, response.c_str(), response.size(), 0);
	close(client_fd);
}

static void respond_text_error(
	int client_fd,
	int status,
	const std::string &title,
	const std::string &message
)
{
	std::ostringstream response;

	response << "HTTP/1.1 "
			 << status
			 << " "
			 << title
			 << "\r\n"
			 << "Content-Type: text/plain\r\n"
			 << "Content-Length: "
			 << message.size()
			 << "\r\n"
			 << "Connection: close\r\n\r\n"
			 << message;

	std::string out = response.str();

	send(client_fd, out.c_str(), out.size(), 0);
	close(client_fd);
}

// ===== Multipart parsing =====

static std::string to_lower_copy(const std::string &value)
{
	std::string lowered = value;
	for (size_t i = 0; i < lowered.size(); ++i)
		lowered[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[i])));
	return lowered;
}

static std::string trim_copy(const std::string &value)
{
	size_t start = 0;
	while (start < value.size() && (value[start] == ' ' || value[start] == '\t' || value[start] == '\r' || value[start] == '\n'))
		++start;

	size_t end = value.size();
	while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r' || value[end - 1] == '\n'))
		--end;

	return value.substr(start, end - start);
}

static bool extract_header_value(const std::string &headers_part, const std::string &name, std::string &value)
{
	std::string lower_headers = to_lower_copy(headers_part);
	std::string lower_name = to_lower_copy(name);
	size_t header_pos = lower_headers.find(lower_name + ":");
	if (header_pos == std::string::npos)
		return false;

	size_t colon_pos = headers_part.find(':', header_pos);
	if (colon_pos == std::string::npos)
		return false;

	size_t start = colon_pos + 1;
	while (start < headers_part.size() && (headers_part[start] == ' ' || headers_part[start] == '\t'))
		++start;

	size_t end = headers_part.find("\r\n", start);
	if (end == std::string::npos)
		end = headers_part.find("\n", start);
	if (end == std::string::npos)
		end = headers_part.size();

	value = trim_copy(headers_part.substr(start, end - start));
	return !value.empty();
}

static std::string extract_boundary(const std::string &headers_part)
{
    std::string content_type;
    if (!extract_header_value(headers_part, "Content-Type", content_type))
        return "";

    std::istringstream ss(content_type);
    std::string token;
    while (std::getline(ss, token, ';'))
    {
        token = trim_copy(token);
        std::string lower = to_lower_copy(token);
        if (lower.find("boundary=") == 0)
        {
            std::string b = trim_copy(token.substr(9));
            if (!b.empty() && b.front() == '"' && b.back() == '"')
                b = b.substr(1, b.size() - 2);
            if (!b.empty())
            {
                if (b.size() >= 2 && b[0] == '-' && b[1] == '-')
                    return b;
                return "--" + b;
            }
        }
    }
    return "";
}

static size_t extract_content_length(const std::string &headers_part)
{
	std::string len_str;
	if (!extract_header_value(headers_part, "Content-Length", len_str))
		return 0;

	std::istringstream iss(len_str);

	size_t value = 0;
	iss >> value;

	return value;
}

static bool split_headers_and_body(
	const std::string &request,
	std::string &headers,
	std::string &body
)
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

// ===== Save uploaded files =====
int save_multipart_files(const std::string &body, const std::string &boundary, const std::string &www_root)
{
    int saved_files = 0;
    if (boundary.empty())
        return 0;

    std::string marker = boundary;
    size_t part_start = body.find(marker);
    if (part_start == std::string::npos)
        return 0;

    while (part_start != std::string::npos)
    {
        part_start += marker.size();

        if (body.compare(part_start, 2, "--") == 0)
            break;

        if (body.compare(part_start, 2, "\r\n") == 0)
            part_start += 2;

        size_t part_end = body.find(marker, part_start);
        if (part_end == std::string::npos)
            break;

        std::string part = body.substr(part_start, part_end - part_start);

        while (!part.empty() && (part.front() == '\r' || part.front() == '\n'))
            part.erase(0, 1);
        while (!part.empty() && (part.back() == '\r' || part.back() == '\n'))
            part.pop_back();

        std::string filename;
        std::string content;

        std::cout << "!!!!Processing multipart part of size: " << part.size() << std::endl;

        if (extract_multipart_file(part, filename, content))
        {
            std::cout << "Saving uploaded file: " << filename << " (" << content.size() << " bytes)" << std::endl;
            if (save_uploaded_file(www_root, filename, content))
                ++saved_files;
        }

        part_start = part_end;
    }

    return saved_files;
}

// ===== Main POST upload handler =====

void Server::handle_post_upload(
	int client_fd,
	const std::string &path,
	const std::string &request_id,
	const std::string &request,
	const std::string &www_root
)
{
	std::cout << "POST "
			  << path
			  << " ("
			  << request_id
			  << ")"
			  << std::endl;

	std::string headers_part;
	std::string buffered_body;

	if (!split_headers_and_body(request, headers_part, buffered_body))
	{
		respond_text_error(
			client_fd,
			400,
			"Bad Request",
			"Malformed request"
		);
		return;
	}

	size_t content_length = extract_content_length(headers_part);

	if (
		content_length == 0
		&& headers_part.find("Content-Length:") == std::string::npos
	)
	{
		respond_text_error(
			client_fd,
			411,
			"Length Required",
			"Content-Length header required"
		);
		return;
	}

	if (content_length > this->_request_data._maxBodySize)
	{
		respond_text_error(
			client_fd,
			413,
			"Payload Too Large",
			"Payload too large.\n"
		);
		return;
	}

	std::string body = buffered_body;

	if (body.size() < content_length)
	{
		body += read_request_body(
			client_fd,
			content_length - body.size()
		);
	}

	if (body.size() < content_length)
	{
		respond_text_error(
			client_fd,
			400,
			"Bad Request",
			"Incomplete request body."
		);
		return;
	}

	if (body.size() > content_length)
		body.erase(content_length);

	std::string boundary = extract_boundary(headers_part);
	std::cerr << "boundary='" << boundary << "' body_size=" << body.size() << " content_length=" << content_length << std::endl;

	if (boundary.empty())
	{
		respond_text_error(
			client_fd,
			400,
			"Bad Request",
			"Invalid multipart form request."
		);
		return;
	}

	int saved_files = save_multipart_files(
		body,
		boundary,
		www_root
	);

	if (saved_files <= 0)
	{
		respond_text_error(
			client_fd,
			400,
			"Bad Request",
			"No valid files found in upload."
		);
		return;
	}

	respond_upload_success(client_fd);
}