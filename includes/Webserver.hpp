
#pragma once

/*
	ALL PRINT COLORS FOR REFERENCE:

	const std::string Color_G = "\033[1;32m";
	const std::string Color_Y = "\033[1;33m";
	const std::string Color_R = "\033[0;31m";
	const std::string Color_NC = "\033[0m";
*/

#include "Config.hpp"
#include <iostream>

#define ARG_ERR "Wrong number of arguments.\nThe correct usage of this program is \"./webserv [config file]\".\nThis argument is not optional."

void		print_cout(std::string printable);
void		print_cerr(std::string printable);

class Webserver
{
	private:
		Config	_config;
		int		_server_socket;
		std::string

	public:
		Webserver();
		// Webserver(const Webserver &other);
		// Webserver &operator=(const Webserver &other);
		~Webserver();

		void	init(int argc, char **argv);
};
