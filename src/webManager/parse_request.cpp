#include "../includes/Server.hpp"

// ============================ 
// URL DECODE
// ============================

static int hex_to_int(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static std::string url_decode(const std::string &input)
{
    std::string out;
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (input[i] == '%' && i + 2 < input.size())
        {
            int hi = hex_to_int(input[i + 1]);
            int lo = hex_to_int(input[i + 2]);
            if (hi >= 0 && lo >= 0)
            {
                out.push_back((hi << 4) | lo);
                i += 2;
                continue;
            }
        }
        out.push_back(input[i]);
    }
    return out;
}


// ============================
// REQUEST ID
// ============================
/**
static std::string build_request_id()
{
    static unsigned long req_counter = 0;
    std::ostringstream ss;
    ss << "req-" << ++req_counter;
    return ss.str();
}**/

// ============================
// EXEC PATH
// ============================
/**
static std::string get_executable_dir()
{
    char exe_path[PATH_MAX];
    std::string dir;

#ifdef __APPLE__
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) == 0)
        dir = dirname(exe_path);
#else
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1)
    {
        exe_path[len] = '\0';
        dir = dirname(exe_path);
    }
#endif

    if (dir.empty())
    {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)))
            dir = cwd;
    }

    return dir;
}
**/


bool parse_request_line(HttpRequest &parsed_request)
{
	// Parse only the first HTTP line: METHOD SP PATH SP VERSION CRLF
	std::istringstream reqstream(parsed_request._req);
	std::string request_line;

	if (!std::getline(reqstream, request_line))
		return false;
	if (!request_line.empty() && request_line[request_line.size() - 1] == '\r')
		request_line.erase(request_line.size() - 1);

	std::istringstream line_stream(request_line);
	if (!(line_stream >> parsed_request._method >> parsed_request._path >> parsed_request._version))
		return false;

	// Reject malformed request lines with extra tokens.
	std::string extra_token;
	if (line_stream >> extra_token)
		return false;

	size_t query_pos = parsed_request._path.find('?');
	if (query_pos != std::string::npos)
		parsed_request._path = parsed_request._path.substr(0, query_pos);

	parsed_request._path = url_decode(parsed_request._path);

	return true;
}