#include "Server.hpp"

/**
 * @brief Validates the HTTP request and sends an appropriate error response if the request is malformed.
 * @param server Reference to the Server instance handling the request.
 * @param parsed_request Reference to the HttpRequest structure containing the parsed request data.
 * @return true if the request is valid and can be processed, false if an error response was sent.
 * This function checks the basic structure of the HTTP request line and ensures it can be parsed correctly.
 * If parsing fails, it sends a 400 Bad Request error page to the client with details about the issue.
 */
bool check_response(Server &server, HttpRequest &parsed_request)
{
    size_t line_end = parsed_request._req.find("\r\n");
    if (line_end == std::string::npos)
        line_end = parsed_request._req.find('\n');
    if (line_end == std::string::npos)
    {
        server.send_error_page(parsed_request._client_fd, 400, "Bad Request", "Malformed request line.", parsed_request._request_id);
        return false;
    }

    std::string request_line = parsed_request._req.substr(0, line_end);
    std::istringstream line_stream(request_line);
    std::string method;
    std::string path;
    std::string version;
    if (!(line_stream >> method >> path >> version))
    {
        server.send_error_page(parsed_request._client_fd, 400, "Bad Request", "Malformed request line.", parsed_request._request_id);
        return false;
    }

    std::string extra_token;
    if (line_stream >> extra_token)
    {
        server.send_error_page(parsed_request._client_fd, 400, "Bad Request", "Malformed request line.", parsed_request._request_id);
        return false;
    }

    if (version != "HTTP/1.1")
    {
        server.send_error_page(parsed_request._client_fd, 505, "HTTP Version Not Supported", "Only HTTP/1.1 is supported.", parsed_request._request_id);
        return false;
    }

    if (parsed_request._req.find("\r\n\r\n") == std::string::npos && parsed_request._req.find("\n\n") == std::string::npos)
    {
        server.send_error_page(parsed_request._client_fd, 400, "Bad Request", "Malformed headers.", parsed_request._request_id);
        return false;
    }

    if (parsed_request._req.find("\nHost:") == std::string::npos && parsed_request._req.find("\r\nHost:") == std::string::npos)
    {
        server.send_error_page(parsed_request._client_fd, 400, "Bad Request", "Host header required.", parsed_request._request_id);
        return false;
    }

    return true;
}

/**
 * @brief Checks if the file at the given path exists and does not exceed the specified maximum size.
 * @param filepath Absolute or relative path to the file to check.
 * @param max_size Maximum allowed file size in bytes.
 * @return true if the file exists and is within the size limit, false otherwise.
 * This function is used to validate file uploads and prevent processing files that are too large or do not exist.
 */
bool checkFileSize(const std::string &filepath, size_t max_size)
{
    std::ifstream file(filepath.c_str(), std::ios::binary | std::ios::ate);
    if (!file)
        return false;

    std::streamsize size = file.tellg();
    if (size < 0)
        return false;
    return (static_cast<size_t>(size) <= max_size);
}