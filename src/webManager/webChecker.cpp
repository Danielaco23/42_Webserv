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
	if (!parse_request_line(parsed_request))
    {
            server.send_error_page(parsed_request._client_fd, 400, "Bad Request", "Malformed request line.", parsed_request._request_id);
        return (false);
    }
    return (true);
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