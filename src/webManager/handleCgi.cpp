#include "Server.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <signal.h>
#include <sstream>
#include <vector>
#include <fcntl.h>


static bool get_request_body(const std::string &request,
                             std::string &body)
{
    size_t pos = request.find("\r\n\r\n");

    if (pos == std::string::npos)
        return false;

    body = request.substr(pos + 4);
    return true;
}

static std::string size_to_string(size_t value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

static bool parse_first_line_target(const std::string &request,
									std::string &target)
{
	size_t line_end = request.find("\r\n");
	if (line_end == std::string::npos)
		return false;

	std::string first_line = request.substr(0, line_end);
	std::istringstream iss(first_line);
	std::string method;
	std::string version;

	if (!(iss >> method >> target >> version))
		return false;

	return true;
}

static bool parse_cgi_target(const std::string &request,
							 std::string &script_path,
							 std::string &query_string)
{
	std::string target;

	if (!parse_first_line_target(request, target))
		return false;

	size_t qpos = target.find('?');
	if (qpos == std::string::npos)
	{
		script_path = target;
		query_string = "";
	}
	else
	{
		script_path = target.substr(0, qpos);
		query_string = target.substr(qpos + 1);
	}
	return true;
}

bool is_cgi_path(const std::string &path, const Location &location)
{
    std::vector<std::string> extensions;
    std::vector<std::string> paths;
    size_t i;

    if (path.find("..") != std::string::npos)
        return false;

    if (path == "/cgi-bin" || path == "/cgi-bin/")
        return true;

    if (path.find("/cgi-bin/") == 0)
        return true;

    extensions = location.get_cgi_extensions();
    for (i = 0; i < extensions.size(); i++)
    {
        if (path.size() >= extensions[i].size()
            && path.compare(
                path.size() - extensions[i].size(),
                extensions[i].size(),
                extensions[i]
            ) == 0)
            return true;
    }

    paths = location.get_cgi_paths();
    for (i = 0; i < paths.size(); i++)
    {
        if (path.find(paths[i]) == 0)
            return true;
    }

    if (path.size() >= 3
        && path.compare(path.size() - 3, 3, ".py") == 0)
        return true;

    if (path.size() >= 3
        && path.compare(path.size() - 3, 3, ".sh") == 0)
        return true;

    return false;
}

static std::string choose_interpreter(const std::string &script_path)
{
	size_t dot = script_path.rfind('.');

	if (dot == std::string::npos)
		return "";

	std::string ext = script_path.substr(dot);

	if (ext == ".py")
		return "/usr/bin/python3";
	if (ext == ".sh")
		return "/bin/bash";

	return "";
}


void Server::handleCgiRead(int fd)
{
    char buffer[4096];
    ssize_t n = read(fd, buffer, sizeof(buffer));

    int client_fd = getCgiClient(fd);

    if (client_fd < 0)
    {
        _pending_remove.push_back(fd);
        return;
    }

    std::map<int, Client>::iterator it = _clients.find(client_fd);

    if (it == _clients.end())
    {
        _pending_remove.push_back(fd);
        return;
    }

    if (n > 0)
    {
        it->second.cgiOutput.append(buffer, n);
        return;
    }

    if (n == 0)
    {
       finishCgi(fd);
        return;
    }

    _pending_remove.push_back(fd);
}

static void add_env(std::vector<std::string> &env,
					const std::string &key,
					const std::string &value)
{
	env.push_back(key + "=" + value);
}

void Server::finishCgi(int pipe_fd)
{
    int client_fd = getCgiClient(pipe_fd);

    if (client_fd < 0)
    {
        _pending_remove.push_back(pipe_fd);
        return;
    }

    std::map<int, Client>::iterator it = _clients.find(client_fd);

    if (it == _clients.end())
    {
        _pending_remove.push_back(pipe_fd);
        return;
    }

	pid_t pid = -1;

    std::map<int, pid_t>::iterator pid_it;
    pid_it = _cgi_pids.find(pipe_fd);

    if (pid_it != _cgi_pids.end())
    {
        pid = pid_it->second;
        _cgi_pids.erase(pid_it);
    }

    if (pid > 0)
        waitpid(pid, NULL, WNOHANG);

    std::string body = it->second.cgiOutput;

    std::ostringstream response;

    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << body;

    it->second.writeBuffer = response.str();
    it->second.state = WRITING;

    enableWriteEvent(client_fd);
    _pending_remove.push_back(pipe_fd);
}

bool handle_cgi_request(Server &server, HttpRequest &request_data,
                        const std::string &root, const Location &location)
{
    std::string script_path;
    std::string query_string;

    if (!parse_cgi_target(request_data._req, script_path, query_string))
        return false;

    std::string body;

    if (!get_request_body(request_data._req, body))
        body = "";

    if (script_path == "/cgi-bin" || script_path == "/cgi-bin/")
    {
        std::string response_body = "CGI endpoint ready\n";
        std::string response;

        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "Content-Length: ";
        response += size_to_string(response_body.size());
        response += "\r\nConnection: close\r\n\r\n";
        response += response_body;

        std::map<int, Client>::iterator it;

        it = server.getClients().find(request_data._client_fd);
        if (it != server.getClients().end())
        {
            it->second.writeBuffer = response;
            it->second.state = WRITING;
            server.enableWriteEvent(request_data._client_fd);
        }
        return true;
    }

    if (request_data._method != "GET"
        && request_data._method != "POST"
        && request_data._method != "DELETE")
    {
        std::string msg;

        msg = "HTTP/1.1 405 Method Not Allowed\r\n";
        msg += "Content-Length: 0\r\n";
        msg += "Connection: close\r\n\r\n";

        std::map<int, Client>::iterator it;

        it = server.getClients().find(request_data._client_fd);
        if (it != server.getClients().end())
        {
            it->second.writeBuffer = msg;
            it->second.state = WRITING;
            server.enableWriteEvent(request_data._client_fd);
        }
        return true;
    }

    std::string interpreter;
    std::string executable;

    interpreter = choose_interpreter(script_path);

    if (!interpreter.empty())
    {
        executable = interpreter;
    }
    else
    {
        std::vector<std::string> cgi_paths;
        std::vector<std::string> cgi_extensions;
        size_t i;

        cgi_paths = location.get_cgi_paths();
        cgi_extensions = location.get_cgi_extensions();

        for (i = 0; i < cgi_extensions.size(); i++)
        {
            if (script_path.size() >= cgi_extensions[i].size()
                && script_path.compare(
                    script_path.size() - cgi_extensions[i].size(),
                    cgi_extensions[i].size(),
                    cgi_extensions[i]) == 0)
            {
                if (!cgi_paths.empty())
                    executable = cgi_paths[0];
                break;
            }
        }
    }

    if (executable.empty())
    {
        std::string msg;

        msg = "HTTP/1.1 415 Unsupported Media Type\r\n";
        msg += "Content-Length: 0\r\n";
        msg += "Connection: close\r\n\r\n";

        std::map<int, Client>::iterator it;

        it = server.getClients().find(request_data._client_fd);
        if (it != server.getClients().end())
        {
            it->second.writeBuffer = msg;
            it->second.state = WRITING;
            server.enableWriteEvent(request_data._client_fd);
        }
        return true;
    }

    std::string location_path;
    std::string relative_path;
    std::string fs_script;

    location_path = location.get_path();

    if (location_path == "/")
        relative_path = script_path;
    else
        relative_path = script_path.substr(location_path.size());

    fs_script = root;

    if (!relative_path.empty())
    {
        if (fs_script[fs_script.size() - 1] != '/'
            && relative_path[0] != '/')
            fs_script += "/";

        fs_script += relative_path;
    }

    struct stat st;

    if (stat(fs_script.c_str(), &st) != 0)
    {
        server.send_error_page(request_data._client_fd, 404,
            "Not Found", "CGI script not found.",
            request_data._request_id);
        return true;
    }

    int in_pipe[2];
    int out_pipe[2];

    if (pipe(in_pipe) != 0)
    {
        server.send_error_page(request_data._client_fd, 500,
            "Internal Server Error", "Pipe error",
            request_data._request_id);
        return true;
    }

    if (pipe(out_pipe) != 0)
    {
        close(in_pipe[0]);
        close(in_pipe[1]);
        server.send_error_page(request_data._client_fd, 500,
            "Internal Server Error", "Pipe error",
            request_data._request_id);
        return true;
    }

    if (fcntl(in_pipe[1], F_SETFL, O_NONBLOCK) < 0)
    {
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        server.send_error_page(request_data._client_fd, 500,
            "Internal Server Error", "fcntl error",
            request_data._request_id);
        return true;
    }

    if (fcntl(out_pipe[0], F_SETFL, O_NONBLOCK) < 0)
    {
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        server.send_error_page(request_data._client_fd, 500,
            "Internal Server Error", "fcntl error",
            request_data._request_id);
        return true;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        server.send_error_page(request_data._client_fd, 500,
            "Internal Server Error", "Fork error",
            request_data._request_id);
        return true;
    }

    if (pid == 0)
    {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);

        std::vector<std::string> env;

        add_env(env, "REQUEST_METHOD", request_data._method);
        add_env(env, "QUERY_STRING", query_string);
        add_env(env, "SERVER_PROTOCOL", request_data._version);
        add_env(env, "PATH_INFO", request_data._path);

        std::vector<char *> envp;

        for (size_t i = 0; i < env.size(); i++)
            envp.push_back(const_cast<char *>(env[i].c_str()));

        envp.push_back(NULL);

        char *argv[3];

        argv[0] = const_cast<char *>(executable.c_str());
        argv[1] = const_cast<char *>(fs_script.c_str());
        argv[2] = NULL;

        execve(executable.c_str(), argv, &envp[0]);
        _exit(1);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    std::map<int, Client>::iterator it;

    it = server.getClients().find(request_data._client_fd);

    if (it != server.getClients().end())
    {
        it->second.cgiInput = body;
        it->second.cgiInputPos = 0;
    }

    server.addCgiPipe(out_pipe[0], request_data._client_fd);
    server.addCgiInputPipe(in_pipe[1], request_data._client_fd);
    server.addCgiPid(out_pipe[0], pid);

    return true;
}
