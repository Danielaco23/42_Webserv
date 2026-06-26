
#pragma once

#ifndef Config_HPP
# define Config_HPP

# include <string>
# include <map>
# include <vector>
# include "Webserver.hpp"
# include "Location.hpp"

/*class Int_Port
{
	private:
		
	public:
		Int_Port(void);
		~Int_Port();
};*/

#define DFLT_NAME "Server"
#define DFLT_MAX_BODY_SIZE 1024
#define DFLT_AUTOINDEX false
#define DFLT_ERRPAGE_NUM 404
#define DFLT_ERRPAGE_ADD "/www/errors/404.html"
#define DFLT_ADDRESS ""

class Config
{
	private:
		bool							_is_real;
		int								_port;		// REQUIRED.
		short							_host[4];	// REQUIRED.
		std::string						_root;		// REQUIRED.
		std::string						_index;		// REQUIRED.

		std::string						_server_name;
		size_t							_client_max_body_size;
		bool							_autoindex;
		std::map<short, std::string>	_error_pages;
		std::vector<Location> 			_locations;
        std::string						_server_address;
        int								_listen_fd;

		void		parse_port(std::string &cntnt);
		void		parse_host(std::string &cntnt);
		void		parse_root(std::string &cntnt);
		void		parse_indx(std::string &cntnt);
		void		parse_optional(std::string &cntnt);
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
