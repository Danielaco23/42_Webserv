#include "Server.hpp"

bool check_response(Server &server, const std::string &req, const std::string &request_id, int client_fd)
{
	HttpRequest parsed_request;
	if (!parse_request_line(req, parsed_request))
    {
            server.send_error_page(client_fd, 400, "Bad Request", "Malformed request line.", request_id);
        return (false);
    }
    return (true);
}