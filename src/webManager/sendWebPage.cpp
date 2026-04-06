#include "Server.hpp"
#include <cerrno>
#include <limits.h>
#include <algorithm>
#include <cctype>

/**
 * @brief Sends a buffer to a socket file descriptor until all bytes are written.
 * @param fd Socket file descriptor.
 * @param buf Buffer containing the data to send.
 * @param len Number of bytes to send from the buffer.
 * @return true if all bytes were sent successfully, false otherwise.
 */
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

/**
 * @brief Sends a string to a socket file descriptor until all bytes are written.
 * @param fd Socket file descriptor.
 * @param s String to send.
 * @return true if all bytes were sent successfully, false otherwise.
 */
static bool sendAll(int fd, const std::string &s)
{
	return sendAll(fd, s.data(), s.size());
}

/**
 * @brief Converts an integer value to string.
 * @param value Numeric value to convert.
 * @return String representation of the given value.
 */
static std::string int_to_string(int value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

/**
 * @brief Reads an entire input stream into a string.
 * @param input Source input stream.
 * @return Full stream contents as a string.
 */
static std::string read_stream(std::istream &input)
{
	std::ostringstream buffer;
	buffer << input.rdbuf();
	return buffer.str();
}

/**
 * @brief Builds a standard HTTP response header block.
 * @param status HTTP status code.
 * @param title HTTP status title.
 * @param content_type Value for the Content-Type header.
 * @param body_size Number of bytes in the response body.
 * @return Complete HTTP headers ending with CRLF CRLF.
 */
static std::string build_headers(int status, const std::string &title, const std::string &content_type, size_t body_size)
{
	std::ostringstream headers;
	headers << "HTTP/1.1 " << status << " " << title << "\r\n"
			<< "Content-Type: " << content_type << "\r\n"
			<< "Content-Length: " << body_size << "\r\n"
			<< "Connection: close\r\n\r\n";
	return headers.str();
}

/**
 * @brief Returns a MIME type based on the file extension.
 * @param path Path of the file being served.
 * @return A MIME type string suitable for the Content-Type header.
 */
static std::string get_mime_type(const std::string &path)
{
	size_t pos = path.rfind('.');
	if (pos == std::string::npos) return "application/octet-stream";
	std::string ext = path.substr(pos + 1);
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

/**
 * @brief Replaces every occurrence of a substring inside a string.
 * @param text String to modify in place.
 * @param from Substring to search for.
 * @param to Replacement text.
 */
static void replace_all(std::string &text, const std::string &from, const std::string &to)
{
	if (from.empty())
		return;

	size_t pos = 0;
	while ((pos = text.find(from, pos)) != std::string::npos)
	{
		text.replace(pos, from.size(), to);
		pos += to.size();
	}
}

/**
 * @brief Chooses the icon and visual class for an HTTP error page.
 * @param status HTTP status code.
 * @param icon Output icon path used by the template.
 * @param visual_class Output CSS class used by the template.
 */
static void set_error_visual(int status, std::string &icon, std::string &visual_class)
{
	icon = "";
	visual_class = "";
	if (status >= 500)
	{
		icon = "/icons/server-error.svg";
		visual_class = "error";
	}
	else if (status == 404)
	{
		icon = "/images/404.avif";
		visual_class = "not-found";
	}
	else if (status >= 400)
	{
		icon = "/icons/client-error.svg";
		visual_class = "error";
	}
}

/**
 * @brief Injects a short suggestion block into the error template for 404 responses.
 * @param html Rendered template HTML.
 */
static void add_not_found_suggestions(std::string &html)
{
	std::string suggest = "<p style=\"margin-top:0;\">Suggestions: check the URL, try <a href=\"/\">home</a>, or search the site.</p>";
	size_t insert_pos = html.find("<div class=\"suggestions\"></div>");
	if (insert_pos != std::string::npos)
		html.replace(insert_pos, 31, "<div class=\"suggestions\">" + suggest + "</div>");
}

/**
 * @brief Sends a static file to the client or falls back to a 404 error page.
 * @param client_fd Socket file descriptor for the client connection.
 * @param filepath Absolute or relative path to the file to serve.
 * @param request_id Identifier used for logging and tracing the request.
 */
void Server::send_file(int client_fd, const std::string &filepath, const std::string &request_id)
{
	std::ifstream file(filepath.c_str(), std::ios::in | std::ios::binary);
	if (!file)
	{
		std::ostringstream why;
		why << "Could not open " << filepath << " (errno=" << errno << ") " << strerror(errno);
		std::cerr << why.str() << " (" << request_id << ")\n";
		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL)
			std::cerr << "CWD: " << cwd << "\n";
		send_error_page(client_fd, 404, "Not Found", "The requested resource was not found.", request_id);
		return ;
	}

	std::string body = read_stream(file);
	std::string mime = get_mime_type(filepath);
	std::string headers = build_headers(200, "OK", mime, body.size());

	sendAll(client_fd, headers);

	if (this->_request_data._method == "HEAD")
    {
		close(client_fd);
		return ;
	}	

	sendAll(client_fd, body);
	close(client_fd);
}

/**
 * @brief Renders the error template and sends it as an HTTP response.
 * @param client_fd Socket file descriptor for the client connection.
 * @param status HTTP status code to return.
 * @param title Short error title shown in the page and response line.
 * @param message Detailed error message shown to the client.
 * @param request_id Identifier used for logging and tracing the request.
 */
void Server::send_error_page(int client_fd, int status, const std::string &title, const std::string &message, const std::string &request_id)
{
	std::string tpl_path;
	if (this->_request_data._www_root.empty())
		tpl_path = "www/errors/template.html";
	else
		tpl_path = this->_request_data._www_root + std::string("/errors/template.html");
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

	std::string html = read_stream(tpl);
	std::string status_str = int_to_string(status);

	replace_all(html, "{{status}}", status_str);
	replace_all(html, "{{title}}", title);
	replace_all(html, "{{message}}", message);
	replace_all(html, "{{request_id}}", request_id);

	std::string icon;
	std::string visual_class;
	set_error_visual(status, icon, visual_class);
	replace_all(html, "{{icon}}", icon);
	replace_all(html, "{{visual_class}}", visual_class);
	replace_all(html, "{{favicon}}", icon);

	if (status == 404)
		add_not_found_suggestions(html);

	std::string header_str = build_headers(status, title, "text/html; charset=UTF-8", html.size());
	sendAll(client_fd, header_str);
	sendAll(client_fd, html);
	close(client_fd);
}