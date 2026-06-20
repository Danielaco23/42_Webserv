
#pragma once

#ifndef Config_HPP
# define Config_HPP

# include <string>
# include <map>
# include <vector>
# include "Webserver.hpp"

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

		unsigned int					_port;		// REQUIRED.
		unsigned short					_host[4];	// REQUIRED.
		std::string						_root;		// REQUIRED.
		std::string						_index;		// REQUIRED.

		std::string						_server_name;
		unsigned long					_client_max_body_size;
		bool							_autoindex;
		std::map<short, std::string>	_error_pages;
		std::vector<std::string> 		_locations;
        std::string						_server_address;
        int								_listen_fd;

	public:
		Config(void);
		Config(std::string config_file);
		~Config();

		class ConfigBadConstructException: public std::exception
		{
			public:
				virtual const char *what() const throw();
		};

};

#endif
