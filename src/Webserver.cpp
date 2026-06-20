
#include "Webserver.hpp"

Webserver::Webserver(Config &config, int server_socket):
_config(config), _server_socket(server_socket)
{}
// Webserver(const Webserver &other);
// Webserver &operator=(const Webserver &other);
Webserver::~Webserver()
{

}
