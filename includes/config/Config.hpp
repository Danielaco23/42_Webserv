
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

typedef std::map<short, std::string> t_err_page;

class Config
{
	private:
		bool							_is_real;	// Indicates if Config has built correctly.
		int								_port;		// REQUIRED.
		std::string						_www_root;	// REQUIRED.
		
		int								_host[4];	// DEFAULTS TO "127.0.0.1"
		std::string						_index;		// DEFAULTS TO "index.html"
		std::string						_server_name;
		size_t							_client_max_body_size;
		bool							_autoindex;
		t_err_page						_error_pages;
		std::vector<Location> 			_locations;
		std::string						_server_address;
		int								_listen_fd;

		void							parse_port(std::string &cntnt);
		void							parse_root(std::string &cntnt);
		void							parse_required(std::string &cntnt);
		
		void							parse_host(std::string &cntnt);
		void							parse_index(std::string &cntnt);
		void							parse_s_name(std::string &cntnt);
		void							parse_cmbs(std::string &cntnt);
		void							parse_autoindex(std::string &cntnt);
		void							parse_error_pages(std::string &cntnt);
		void							parse_locations(std::string &cntnt);
		void							parse_s_address(std::string &cntnt);
		void							parse_optional(std::string &cntnt);

		void							throw_with_msg(std::string msg);

	public:
		Config(void);
		Config(std::string config_file);

		void							add_location(std::string path, std::string content);

		bool							&get_is_real();
		void							set_is_real(bool new_data);
		int								&get_port();
		void							set_port(int new_data);
		int								*get_host();
		void							set_host(int* new_data);
		std::string						&get_root();
		void							set_root(std::string new_data);
		std::string						&get_index();
		void							set_index(std::string new_data);
		std::string						&get_server_name();
		void							set_server_name(std::string new_data);
		size_t							&get_client_max_body_size();
		void							set_client_max_body_size(size_t new_data);
		bool							&get_autoindex();
		void							set_autoindex(bool new_data);
		t_err_page						&get_error_pages();
		void							set_error_pages(t_err_page new_data);
		std::vector<Location>			&get_locations();
		void							set_locations(std::vector<Location> new_data);
		std::string						&get_server_address();
		void							set_server_address(std::string new_data);
		int								&get_listen_fd();
		void							set_listen_fd(int new_data);

		class ConfigBadConstrException: public std::exception
		{
			private:
				std::string		_msg;
			public:
				ConfigBadConstrException(const std::string& message);
				const char* what() const throw();
				~ConfigBadConstrException() _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW;
		};

		~Config();
};

#endif
