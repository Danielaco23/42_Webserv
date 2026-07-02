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
#include <fcntl.h>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>

#include "Client.hpp"
#include "Config_jh.hpp"

extern volatile sig_atomic_t g_running;

/**
 * @brief Listening socket + config association
 */
struct ListeningSocket {
    int fd;
    ServerConfig config;
};

/**
 * @brief Multi-server non-blocking HTTP server (poll-based)
 */
class Server
{
private:
    // listening sockets (MULTI SERVER CORE)
    std::vector<ListeningSocket> _servers;

    // poll fds (listening + clients)
    std::vector<pollfd> _fds;

    std::map<int, Client> _clients;

    std::vector<pollfd> _pending_add;
    std::vector<int> _pending_remove;

    size_t _number_of_clients;

    // core handlers
    void acceptClient(int listening_fd);
    void handleClientRead(int fd);
    void handleClientWrite(int fd);
    void removeClient(int fd);

    void handle_post_upload(int client_fd,
                            const std::string &path,
                            const std::string &request_id,
                            const std::string &request,
                            const std::string &www_root,
                            size_t max_body_size);

    void handle_uploads_listing(int client_fd,
                                const std::string &www_root);

public:
    Server(const std::vector<ServerConfig> &configs);
    ~Server();

    void initSocket();
    void initVariables();
    void startListening(); // opcional (puedes eliminarla si quieres)
    void run();

    void shutdownServer();

    int findServerIndex(int listening_fd);
    bool isListeningSocket(int fd);

    void processPollEvents();
    void applyPendingChanges();

    void send_file(int client_fd,
                   const std::string &filepath,
                   const std::string &request_id);

    void send_error_page(int client_fd,
                        int status,
                        const std::string &title,
                        const std::string &message,
                        const std::string &request_id);
};

void signalHandler(int signal);
bool save_uploaded_file(
    const std::string &www_root,
    const std::string &filename,
    const std::string &content);

bool extract_multipart_file(
    const std::string &part,
    std::string &filename,
    std::string &content);

bool handle_cgi_request(Server &server, HttpRequest &request_data);
bool is_cgi_path(const std::string &path);
bool check_response(Server &server, HttpRequest &request_data);
bool parse_request_line(HttpRequest &request_data);

#endif