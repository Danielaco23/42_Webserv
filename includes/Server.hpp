#ifndef SERVER_HPP
#define SERVER_HPP

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

#include <signal.h>

#include <iostream>
#include <vector>
#include <map>
#include <dirent.h>


#include <limits.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <fcntl.h>
#include <sstream>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <dirent.h>
#include <sys/stat.h>
#include <sstream>


#include "Client.hpp"


extern volatile sig_atomic_t g_running;

/**
 * @brief Simple HTTP server using non-blocking sockets and poll()
 */
class Server
{
    private:
        int _server_fd;
        int _port;
        HttpRequest _request_data;
        struct sockaddr_in _address;

        std::vector<pollfd> _fds;
        std::map<int, Client> _clients;

        // nuevos vectores para manejar clientes pendientes de agregar y eliminar
        std::vector<pollfd> _pending_add;
        std::vector<int> _pending_remove;
        
        void handleRequest();
        void acceptClient();
        void handleClientRead(int fd);
        void handleClientWrite(int fd);
        void removeClient(int fd);
        size_t _number_of_clients;
        void handle_post_upload(...);
        void handle_uploads_listing(...);
        void handle_post_upload(int client_fd,
                            const std::string &path,
                            const std::string &request_id,
                            const std::string &request,
                            const std::string &www_root);

        void handle_uploads_listing(int client_fd,
                                const std::string &www_root);

        // bool receive_request(int client_fd,
        //                     const std::string &request_id,
        //                     std::string &request);

    public:
        Server(int port);
        ~Server();

        std::string _www_root;
        void initSocket();
        void initVariables();
        void startListening();
        void run();

        void sendWebPage(int client_fd);

        int getServerFd() const;

        void send_file(
            int client_fd,
            const std::string &filepath,
            const std::string &request_id
        );

        void send_error_page(
            int client_fd,
            int status,
            const std::string &title,
            const std::string &message,
            const std::string &request_id
        );
        void shutdownServer();//signal
};

void signalHandler(int signal);
bool handle_cgi_request(Server &server, HttpRequest &request_data);
bool is_cgi_path(const std::string &path);
bool check_response(Server &server, HttpRequest &request_data);
bool parse_request_line(HttpRequest &request_data);
bool save_uploaded_file(const std::string &www_root, const std::string &filename, const std::string &content);
bool extract_multipart_file(const std::string &part, std::string &filename, std::string &content);
std::string read_request_body(int client_fd, size_t content_length);

#endif