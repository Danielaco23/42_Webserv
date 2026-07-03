#pragma once

#ifndef Config_HPP
# define Config_HPP

# include <string>
# include <map>
# include <vector>
# include "Webserver.hpp"
# include "Location.hpp"

typedef std::map<short, std::string> t_err_page;

class Config
{
	private:
		bool							_is_real;
		int								_port;
		std::string						_www_root;

		int								_host[4];
		std::string						_index;
		std::string						_server_name;
		size_t							_client_max_body_size;
		bool							_autoindex;
		t_err_page						_error_pages;
		std::vector<Location>			_locations;
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

		void							add_location(std::string path,
										std::string content);

		bool							&get_is_real();
		void							set_is_real(bool new_data);

		int								&get_port();
		void							set_port(int new_data);

		int								*get_host();
		void							set_host(int *new_data);

		const std::string				&get_root() const;
		void							set_root(std::string new_data);

		const std::string				&get_index() const;
		void							set_index(std::string new_data);

		const std::string				&get_server_name() const;
		void							set_server_name(std::string new_data);

		const size_t					&get_client_max_body_size() const;
		void							set_client_max_body_size(size_t new_data);

		const bool						&get_autoindex() const;
		void							set_autoindex(bool new_data);

		const t_err_page				&get_error_pages() const;
		void							set_error_pages(t_err_page new_data);

		const std::vector<Location>		&get_locations() const;
		void							set_locations(std::vector<Location> new_data);

		const std::string				&get_server_address() const;
		void							set_server_address(std::string new_data);

		int								&get_listen_fd();
		void							set_listen_fd(int new_data);

		class ConfigBadConstrException : public std::exception
		{
			private:
				std::string		_msg;

			public:
				ConfigBadConstrException(const std::string& message);
				const char		*what() const throw();
				~ConfigBadConstrException()
					_GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW;
		};

		~Config();
};

#endif