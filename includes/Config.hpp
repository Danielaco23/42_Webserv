
#pragma once

#ifndef Config_HPP
# define Config_HPP

# include "Webserver.hpp"
# include <map>
# include <vector>
/*class Int_Port
{
	private:
		
	public:

		Int_Port(void);
		~Int_Port();

};*/

class Config
{
	private:

		unsigned int					_port;  // REQUIRED.
		unsigned short					_host;  // REQUIRED.
		std::string						_root;  // REQUIRED.
		std::string						_index; // REQUIRED.

		std::string						_server_name;
		unsigned long					_client_max_body_size;
		bool							_autoindex;
		std::map<short, std::string>	_error_pages;
		std::vector<std::string> 		_locations;
        std::string						_server_address;
        int     						_listen_fd;

	public:

		Config(void);
		Config(Config& original);
		~Config();

};

Config::Config(){};
Config::Config(Config& original){};
Config::~Config(){};
#endif
