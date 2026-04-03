#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <cstdlib>  // exit, EXIT_FAILURE
#include <cstdio>   // perror
#include <cstring> //memset
#include <unistd.h> //close
#include <arpa/inet.h> //socket, bind , listen, accept

#include <fstream>
#include <sstream>
#include <string>

class Server
{
	private:
		int                 _server_fd; // fd socket
		int                 _port;
		struct sockaddr_in  _address; //ip, port, Af_inet
		std::string         _www_root; // absolute path to www directory (computed from exe)
		int				 	_number_of_clients;	// NO SE SI VA A IR AQUI, PERO DE MOMENTO AQUI LO DEJO

	public:
		Server(int port);
		~Server();

		void initSocket();
		void startListening();
		void acceptConnection();

		int getServerFd() const;
		void send_file(int client_fd, const std::string &filepath, const std::string &request_id);
		void send_error_page(int client_fd, int status, const std::string &title, const std::string &message, const std::string &request_id);
};
// Upload helpers
std::string read_request_body(int client_fd, size_t content_length);
bool save_uploaded_file(const std::string &www_root, const std::string &filename, const std::string &content);
bool extract_multipart_file(const std::string &part, std::string &filename, std::string &content);
int process_uploads(const std::string &body, const std::string &boundary, const std::string &www_root);
#endif