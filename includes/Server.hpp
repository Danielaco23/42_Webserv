#ifndef SERVER_HPP
#define SERVER_HPP

#include <cstdlib>  // exit, EXIT_FAILURE
#include <cstdio>   // perror
#include <cstring> //memset
#include <unistd.h> //close
#include <arpa/inet.h> //socket, bind , listen, accept
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
#include <vector>

# define GREEN "\033[0;32m"
# define WHITE "\033[37m"
# define RED "\033[0;31m\033[1m"
# define ORANGE "\001\033[38;5;208m\002"
# define BLUE "\033[0;34m"
# define PURPLE "\033[0;35m"
# define CYAN "\033[0;36m"
# define YELLOW  "\x1b[33m"
# define ROSE    "\x1B[38;2;255;151;203m"
# define LIGHT_BLUE   "\x1B[38;2;53;149;240m"
# define LIGHT_GREEN  "\x1B[38;2;17;245;120m"
# define GRAY    "\x1B[38;2;176;174;174m"
# define NC "\033[0m"

struct HttpRequest
{
	std::string 	_method;
	std::string 	_file_path;
	std::string		_req;
	std::string		_request_id;
	std::string 	_path;
	std::string		_version;
	size_t			_maxBodySize;
	std::string     _www_root; // absolute path to www directory (computed from exe)
	int				_client_fd;
};

class Server
{
	private:
		int                 _server_fd; // fd socket
		int                 _port;
		struct sockaddr_in  _address; //ip, port, Af_inet
		int				 	_number_of_clients;	// NO SE SI VA A IR AQUI, PERO DE MOMENTO AQUI LO DEJO
		HttpRequest			_request_data;
	public:
		Server(int port);
		~Server();

		void 	initSocket();
		void 	initVariables();
		void 	startListening();
		void	acceptConnection();
		void	handleRequest();

		int getServerFd() const;
		void send_file(int client_fd, const std::string &filepath, const std::string &request_id);
		void send_error_page(int client_fd, int status, const std::string &title, const std::string &message, const std::string &request_id);
};

std::string		read_request_body(int client_fd, size_t content_length);
bool			save_uploaded_file(const std::string &www_root, const std::string &filename, const std::string &content);
bool 			extract_multipart_file(const std::string &part, std::string &filename, std::string &content);
int 			process_uploads(const std::string &body, const std::string &boundary, const std::string &www_root);
std::string 	build_uploads_json(const std::string &www_root);

void			handle_post_upload(int client_fd, const std::string &path, const std::string &request_id, const std::string &request, const std::string &www_root);
void			handle_uploads_listing(int client_fd, const std::string &www_root);
bool			handle_cgi_request(Server &server, HttpRequest &request_data);
bool			parse_request_line(HttpRequest &parsed_request);
bool			check_response(Server &server, HttpRequest &parsed_request);

#endif