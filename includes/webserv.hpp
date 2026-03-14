
#pragma once

/*
	ALL PRINT COLORS FOR REFERENCE:

	const std::string G_color_G = "\033[1;32m";
	const std::string G_color_Y = "\033[1;33m";
	const std::string G_color_R = "\033[0;31m";
	const std::string G_color_NC = "\033[0m";
*/

#include "Config.hpp"
#include <iostream>

#define ARG_ERR "Wrong number of arguments.\nThe correct usage of this program is \"./webserv [config file]\".\nThis argument is not optional."

void		print_cout(std::string printable);
void		print_cerr(std::string printable);