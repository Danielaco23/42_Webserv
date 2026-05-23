#include "Server.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <signal.h>
#include <sstream>
#include <vector>
#include <fcntl.h>

static bool send_all(int fd, const std::string &data)
{
	size_t sent = 0;
	while (sent < data.size())
	{
		ssize_t n = send(fd, data.c_str() + sent, data.size() - sent, 0);
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			return false;
		}
		if (n == 0)
			return false;
		sent += static_cast<size_t>(n);
	}
	return true;
}

static std::string size_to_string(size_t value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

static bool parse_first_line_target(const std::string &request, std::string &target)
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

static bool parse_cgi_target(const std::string &request, std::string &script_path, std::string &query_string)
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

static bool is_cgi_path(const std::string &path)
{
	if (path == "/cgi-bin" || path == "/cgi-bin/")
		return true;
	if (path.find("/cgi-bin/") != 0)
		return false;
	if (path.find("..") != std::string::npos)
		return false;
	return true;
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

static std::string read_cgi_output(int out_fd)
{
	std::string output;
	char buffer[4096];

	while (true)
	{
		ssize_t n = read(out_fd, buffer, sizeof(buffer));
		if (n <= 0)
			break;
		output.append(buffer, n);
	}
	return output;
}

static void add_env(std::vector<std::string> &env, const std::string &key, const std::string &value)
{
	env.push_back(key + "=" + value);
}

static void send_cgi_http_response(int client_fd, const std::string &cgi_output)
{
	std::string body = cgi_output;

	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: " + size_to_string(body.size()) + "\r\n"
		"Connection: close\r\n\r\n" + body;

	send_all(client_fd, response);
	close(client_fd);
}

bool handle_cgi_request(Server &server, HttpRequest &request_data)
{
	(void)server;

	std::string script_path;
	std::string query_string;

	if (!parse_cgi_target(request_data._req, script_path, query_string))
		return false;

	if (!is_cgi_path(script_path))
		return false;

	if (script_path == "/cgi-bin" || script_path == "/cgi-bin/")
	{
		std::string body = "CGI endpoint ready\n";

		std::string response =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: " + size_to_string(body.size()) + "\r\n"
			"Connection: close\r\n\r\n" + body;

		send_all(request_data._client_fd, response);
		close(request_data._client_fd);
		return true;
	}

	if (request_data._method != "GET" &&
		request_data._method != "POST" &&
		request_data._method != "DELETE")
	{
		std::string msg =
			"HTTP/1.1 405 Method Not Allowed\r\n"
			"Content-Length: 0\r\n"
			"Connection: close\r\n\r\n";

		send_all(request_data._client_fd, msg);
		close(request_data._client_fd);
		return true;
	}

	std::string interpreter = choose_interpreter(script_path);
	if (interpreter.empty())
	{
		std::string msg =
			"HTTP/1.1 415 Unsupported Media Type\r\n"
			"Content-Length: 0\r\n"
			"Connection: close\r\n\r\n";

		send_all(request_data._client_fd, msg);
		close(request_data._client_fd);
		return true;
	}

	std::string fs_script = "." + script_path;

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

	if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0)
	{
		server.send_error_page(request_data._client_fd, 500,
			"Internal Server Error", "Pipe error",
			request_data._request_id);
		return true;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		server.send_error_page(request_data._client_fd, 500,
			"Internal Server Error", "Fork error",
			request_data._request_id);
		return true;
	}

	if (pid == 0)
	{
		dup2(in_pipe[0], STDIN_FILENO);
		dup2(out_pipe[1], STDOUT_FILENO);

		close(in_pipe[1]);
		close(out_pipe[0]);

		std::vector<std::string> env;
		add_env(env, "REQUEST_METHOD", request_data._method);
		add_env(env, "QUERY_STRING", query_string);

		std::vector<char *> envp;
		for (size_t i = 0; i < env.size(); i++)
			envp.push_back(const_cast<char *>(env[i].c_str()));
		envp.push_back(NULL);

		char *argv[3];
		argv[0] = const_cast<char *>(interpreter.c_str());
		argv[1] = const_cast<char *>(fs_script.c_str());
		argv[2] = NULL;

		execve(interpreter.c_str(), argv, &envp[0]);
		_exit(1);
	}

	close(in_pipe[0]);
	close(out_pipe[1]);

	std::string cgi_output = read_cgi_output(out_pipe[0]);
	close(out_pipe[0]);

	send_cgi_http_response(request_data._client_fd, cgi_output);
	return true;
}