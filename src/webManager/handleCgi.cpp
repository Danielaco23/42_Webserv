#include "Server.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <signal.h>

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

static bool starts_with(const std::string &value, const std::string &prefix)
{
	return value.compare(0, prefix.size(), prefix) == 0;
}

static std::string int_to_string(int value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

static std::string size_to_string(size_t value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

static std::string get_header_value(const std::string &headers, const std::string &name)
{
	size_t pos = headers.find(name + ":");
	if (pos == std::string::npos)
		return "";

	pos += name.size() + 1;
	while (pos < headers.size() && (headers[pos] == ' ' || headers[pos] == '\t'))
		++pos;

	size_t end = headers.find("\r\n", pos);
	if (end == std::string::npos)
		end = headers.find('\n', pos);
	if (end == std::string::npos)
		end = headers.size();
	return headers.substr(pos, end - pos);
}

static bool parse_first_line_target(const std::string &request, std::string &target)
{
	size_t line_end = request.find("\r\n");
	if (line_end == std::string::npos)
		line_end = request.find('\n');
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
	if (!starts_with(path, "/cgi-bin/"))
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

static bool split_headers_body(const std::string &raw, std::string &headers, std::string &body)
{
	size_t pos = raw.find("\r\n\r\n");
	size_t sep_len = 4;
	if (pos == std::string::npos)
	{
		pos = raw.find("\n\n");
		sep_len = 2;
	}
	if (pos == std::string::npos)
		return false;

	headers = raw.substr(0, pos);
	body = raw.substr(pos + sep_len);
	return true;
}

static std::string read_cgi_output(int out_fd)
{
	std::string output;
	char buffer[4096];
	while (true)
	{
		ssize_t n = read(out_fd, buffer, sizeof(buffer));
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			break;
		}
		if (n == 0)
			break;
		output.append(buffer, static_cast<size_t>(n));
	}
	return output;
}

static void add_env(std::vector<std::string> &env, const std::string &key, const std::string &value)
{
	env.push_back(key + "=" + value);
}

static void send_cgi_http_response(int client_fd, const std::string &cgi_output)
{
	std::string headers;
	std::string body;
	if (!split_headers_body(cgi_output, headers, body))
	{
		std::string fallback = "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=UTF-8\r\nContent-Length: "
			+ size_to_string(cgi_output.size()) + "\r\nConnection: close\r\n\r\n" + cgi_output;
		send_all(client_fd, fallback);
		close(client_fd);
		return;
	}

	int status = 200;
	std::string title = "OK";
	std::string content_type = "text/html; charset=UTF-8";

	std::istringstream hs(headers);
	std::string line;
	while (std::getline(hs, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (starts_with(line, "Status:"))
		{
			std::string status_line = line.substr(7);
			while (!status_line.empty() && (status_line[0] == ' ' || status_line[0] == '\t'))
				status_line.erase(0, 1);
			std::istringstream ss(status_line);
			ss >> status;
			std::getline(ss, title);
			if (!title.empty() && title[0] == ' ')
				title.erase(0, 1);
			if (title.empty())
				title = "OK";
		}
		else if (starts_with(line, "Content-Type:"))
		{
			content_type = line.substr(13);
			while (!content_type.empty() && (content_type[0] == ' ' || content_type[0] == '\t'))
				content_type.erase(0, 1);
		}
	}

	std::string response = "HTTP/1.1 " + int_to_string(status) + " " + title + "\r\n"
		+ "Content-Type: " + content_type + "\r\n"
		+ "Content-Length: " + size_to_string(body.size()) + "\r\n"
		+ "Connection: close\r\n\r\n" + body;
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
		std::string body = "CGI endpoint ready. Use /cgi-bin/<script>.py or /cgi-bin/<script>.sh\n";
		std::string response = "HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain; charset=UTF-8\r\n"
			"Content-Length: " + size_to_string(body.size()) + "\r\n"
			"Connection: close\r\n\r\n" + body;
		send_all(request_data._client_fd, response);
		close(request_data._client_fd);
		return true;
	}

	if (request_data._method != "GET" && request_data._method != "POST" && request_data._method != "DELETE")
	{
		std::string msg = "HTTP/1.1 405 Method Not Allowed\r\nAllow: GET, POST, DELETE\r\nContent-Type: text/plain; charset=UTF-8\r\nContent-Length: 24\r\nConnection: close\r\n\r\nMethod not allowed for CGI";
		send_all(request_data._client_fd, msg);
		close(request_data._client_fd);
		return true;
	}

	std::string interpreter = choose_interpreter(script_path);
	if (interpreter.empty())
	{
		std::string msg = "HTTP/1.1 415 Unsupported Media Type\r\nContent-Type: text/plain; charset=UTF-8\r\nContent-Length: 37\r\nConnection: close\r\n\r\nUnsupported CGI script extension.\n";
		send_all(request_data._client_fd, msg);
		close(request_data._client_fd);
		return true;
	}

	std::string fs_script = "." + script_path;
	struct stat st;
	if (stat(fs_script.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
	{
		server.send_error_page(request_data._client_fd, 404, "Not Found", "CGI script not found.", request_data._request_id);
		return true;
	}

	std::string headers_part;
	std::string buffered_body;
	if (!split_headers_body(request_data._req, headers_part, buffered_body))
	{
		server.send_error_page(request_data._client_fd, 400, "Bad Request", "Malformed HTTP headers.", request_data._request_id);
		return true;
	}

	size_t content_length = 0;
	std::string cl = get_header_value(headers_part, "Content-Length");
	if (!cl.empty())
	{
		std::istringstream ss(cl);
		ss >> content_length;
	}

	std::string body = buffered_body;
	if (content_length > body.size())
		body += read_request_body(request_data._client_fd, content_length - body.size());
	if (body.size() > content_length)
		body.erase(content_length);

	int in_pipe[2];
	int out_pipe[2];
	if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0)
	{
		server.send_error_page(request_data._client_fd, 500, "Internal Server Error", "Failed to create CGI pipes.", request_data._request_id);
		return true;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		close(in_pipe[0]);
		close(in_pipe[1]);
		close(out_pipe[0]);
		close(out_pipe[1]);
		server.send_error_page(request_data._client_fd, 500, "Internal Server Error", "Failed to fork CGI process.", request_data._request_id);
		return true;
	}

	if (pid == 0)
	{
		dup2(in_pipe[0], STDIN_FILENO);
		dup2(out_pipe[1], STDOUT_FILENO);
		dup2(out_pipe[1], STDERR_FILENO);

		close(in_pipe[1]);
		close(out_pipe[0]);
		close(in_pipe[0]);
		close(out_pipe[1]);

		std::vector<std::string> env;
		add_env(env, "GATEWAY_INTERFACE", "CGI/1.1");
		add_env(env, "SERVER_PROTOCOL", request_data._version.empty() ? "HTTP/1.1" : request_data._version);
		add_env(env, "REQUEST_METHOD", request_data._method);
		add_env(env, "SCRIPT_NAME", script_path);
		add_env(env, "PATH_INFO", script_path);
		add_env(env, "QUERY_STRING", query_string);
		add_env(env, "CONTENT_LENGTH", size_to_string(body.size()));
		add_env(env, "CONTENT_TYPE", get_header_value(headers_part, "Content-Type"));
		add_env(env, "SERVER_SOFTWARE", "webserv/0.1");

		std::vector<char *> envp;
		for (size_t i = 0; i < env.size(); ++i)
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

	if (!body.empty())
		send_all(in_pipe[1], body);
	close(in_pipe[1]);

	int status = 0;
	int timeout_sec = 5;
	int elapsed = 0;
	int exit_status = -1;

	while (elapsed < timeout_sec)
	{
		pid_t wait_result = waitpid(pid, &status, WNOHANG);
		if (wait_result == pid)
		{
			if (WIFEXITED(status))
				exit_status = WEXITSTATUS(status);
			break;
		}
		if (wait_result < 0)
		{
			perror("waitpid");
			break;
		}
		sleep(1);
		++elapsed;
	}

	if (exit_status < 0)
	{
		kill(pid, SIGTERM);
		usleep(200000);
		if (waitpid(pid, NULL, WNOHANG) == 0)
		{
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
		}
		close(out_pipe[0]);
		server.send_error_page(request_data._client_fd, 504, "Gateway Timeout", "CGI script timeout.", request_data._request_id);
		return true;
	}

	std::string cgi_output = read_cgi_output(out_pipe[0]);
	close(out_pipe[0]);

	if (exit_status != 0)
	{
		server.send_error_page(request_data._client_fd, 502, "Bad Gateway", "CGI process failed.", request_data._request_id);
		return true;
	}

	send_cgi_http_response(request_data._client_fd, cgi_output);
	return true;
}
