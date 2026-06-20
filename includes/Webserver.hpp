
#pragma once

/*
	ALL PRINT COLORS FOR REFERENCE:

	const std::string GREEN = "\033[0;32m"
	const std::string RED = "\033[0;31m\033[1m"
	const std::string ORANGE = "\001\033[38;5;208m\002"
	const std::string BLUE = "\033[0;34m"
	const std::string PURPLE = "\033[0;35m"
	const std::string CYAN = "\033[0;36m"
	const std::string YELLOW = "\033[33m"
	const std::string ROSE = "\x1B[38;2;255;151;203m"
	const std::string LIGHT_BLUE = "\x1B[38;2;53;149;240m"
	const std::string LIGHT_GREEN = "\x1B[38;2;17;245;120m"
	const std::string GRAY = "\x1B[38;2;176;174;174m"
	const std::string NC = "\033[0m"
*/

class Config;

#include <iostream>
#include <sstream>
#include <fstream>
#include "Config.hpp"

#define ARG_ERR "Wrong number of arguments.\nThe correct usage of this program is \"./webserv [config file]\".\nThis argument is not optional."

class Webserver
{
	private:
		Config			&_config;
		int				_server_socket;

	public:
		Webserver(Config &config, int server_socket);
		// Webserver(const Webserver &other);
		// Webserver &operator=(const Webserver &other);
		~Webserver();

		void	init(int argc, char **argv);
};
