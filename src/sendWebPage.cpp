#include "../includes/Server.hpp"
#include <cerrno>
#include <limits.h>
#include <algorithm>
#include <cctype>

static bool sendAll(int fd, const char *buf, size_t len)
{
	size_t sent = 0;
	while (sent < len)
	{
		ssize_t n = send(fd, buf + sent, static_cast<size_t>(len - sent), 0);
		if (n < 0)
		{
			if (errno == EINTR) continue;
			return false;
		}
		if (n == 0) return false;
		sent += static_cast<size_t>(n);
	}
	return true;
}

static bool sendAll(int fd, const std::string &s)
{
	return sendAll(fd, s.data(), s.size());
}

// simple mime-type detection by extension
static std::string get_mime_type(const std::string &path)
{
	size_t pos = path.rfind('.');
	if (pos == std::string::npos) return "application/octet-stream";
	std::string ext = path.substr(pos + 1);
	// lowercase
	std::transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int))std::tolower);

	if (ext == "html" || ext == "htm") return "text/html; charset=UTF-8";
	if (ext == "css") return "text/css; charset=UTF-8";
	if (ext == "js") return "application/javascript; charset=UTF-8";
	if (ext == "json") return "application/json; charset=UTF-8";
	if (ext == "svg") return "image/svg+xml";
	if (ext == "png") return "image/png";
	if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
	if (ext == "gif") return "image/gif";
	if (ext == "webp") return "image/webp";
	if (ext == "avif") return "image/avif";
	return "application/octet-stream";
}

// send arbitrary file; if not found or cannot open, send error page
void Server::send_file(int client_fd, const std::string &filepath, const std::string &path, const std::string &request_id)
{
	std::ifstream file(filepath.c_str(), std::ios::in | std::ios::binary);
	if (!file)
	{
		std::ostringstream why;
		why << "!!! Could not open " << filepath << " (errno=" << errno << ") " << strerror(errno);
		std::cerr << why.str() << " (" << request_id << ")\n";
		// log current working directory to help debug relative path issues
		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL)
			std::cerr << "CWD: " << cwd << "\n";
		send_error_page(client_fd, 404, "Not Found", request_id);
		return ;
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	std::string body = buffer.str();

	std::ostringstream lenoss;
	lenoss << body.size();
	std::string len = lenoss.str();

	std::string mime = get_mime_type(filepath);
	std::string headers;
	headers  = "HTTP/1.1 200 OK\r\n";
	headers += "Content-Type: ";
	headers += mime;
	headers += "\r\n";
	headers += "Content-Length: ";
	headers += len;
	headers += "\r\n";
	headers += "Connection: close\r\n";
	headers += "\r\n";

	sendAll(client_fd, headers);
	sendAll(client_fd, body);
	close(client_fd);
}

// render the error template and send it
void Server::send_error_page(int client_fd, int status, const std::string &title, const std::string &message, const std::string &request_id)
{
	std::string tpl_path = _www_root.empty() ? std::string("www/errors/template.html") : (_www_root + std::string("/errors/template.html"));
	std::ifstream tpl(tpl_path.c_str());
	if (!tpl)
	{
		std::ostringstream fallback;
		fallback << "HTTP/1.1 " << status << " " << title << "\r\n"
				 << "Content-Type: text/plain; charset=UTF-8\r\n"
				 << "Content-Length: " << message.size() + 50 << "\r\n"
				 << "Connection: close\r\n\r\n"
				 << "Error " << status << " - " << title << "\n" << message << "\n" << "Request id: " << request_id;
		std::string out = fallback.str();
		sendAll(client_fd, out);
		close(client_fd);
		return;
	}

	std::ostringstream s;
	s << tpl.rdbuf();
	std::string html = s.str();

	std::string status_str;
	{
		std::ostringstream ss; 
		ss << status; 
		status_str = ss.str();
	}

	size_t pos;
	while ((pos = html.find("{{status}}")) != std::string::npos)
		html.replace(pos, 10, status_str);
	while ((pos = html.find("{{title}}")) != std::string::npos)
		html.replace(pos, 9, title);
	while ((pos = html.find("{{message}}")) != std::string::npos)
		html.replace(pos, 11, message);
	while ((pos = html.find("{{request_id}}")) != std::string::npos)
		html.replace(pos, 14, request_id);

	// select icon and visual class based on status (defaults already created earlier in other edits)
	std::string icon = "/icons/error.svg";
	std::string visual_class = "";
	if (status >= 500)
	{
		icon = "/icons/server-error.svg";
		visual_class = "error";
	}
	else if (status >= 400)
	{
		icon = "/icons/client-error.svg";
		visual_class = "error";
	}

	while ((pos = html.find("{{icon}}")) != std::string::npos)
		html.replace(pos, 8, "images/404.avif");
	while ((pos = html.find("{{visual_class}}")) != std::string::npos)
		html.replace(pos, 16, visual_class);
	while ((pos = html.find("{{favicon}}")) != std::string::npos)
		html.replace(pos, 11, icon);

	if (status == 404)
	{
		std::string suggest = "<p style=\"margin-top:12px;color:#5b6b86;\">Suggestions: check the URL, try <a href=\"/\">home</a>, or search the site.</p>";
		// insert before </article> to match the template structure
		size_t insert_pos = html.find("</article>");
		if (insert_pos != std::string::npos)
		{
			// avoid duplicate insertion
			if (html.find("Suggestions: check the URL") == std::string::npos)
				html.insert(insert_pos, suggest);
		}
	}

	std::ostringstream headers;
	headers << "HTTP/1.1 " << status << " " << title << "\r\n"
			<< "Content-Type: text/html; charset=UTF-8\r\n"
			<< "Content-Length: " << html.size() << "\r\n"
			<< "Connection: close\r\n\r\n";

	std::string header_str = headers.str();
	sendAll(client_fd, header_str);
	sendAll(client_fd, html);
	close(client_fd);
}