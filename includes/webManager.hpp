#ifndef WEBMANAGER_HPP
#define WEBMANAGER_HPP

#include <string>

// Devuelve JSON con los uploads
std::string build_uploads_json(const std::string &uploads_dir);

// Handler HTTP
void handle_uploads_listing(int client_fd, const std::string &www_root);

#endif