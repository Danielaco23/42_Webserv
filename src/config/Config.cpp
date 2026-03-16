#include "Config.hpp"

LocationConfig::LocationConfig()
{
	path = "/";
	root = "";
	autoindex = false;
	cgi_extension = "";
	cgi_path = "";
	cgi_path = "";
	upload_path = "";
}

ServerConfig::ServerConfig()
{
	host = "0.0.0.0";
	port = 80;
	root = "";
	client_max_body_size = 1000000;
}

Config::Config()
{
}
