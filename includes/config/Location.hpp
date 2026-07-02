#pragma once

#ifndef LOCATION_HPP
# define LOCATION_HPP

# include <cstring>

# include "Webserver.hpp"

typedef std::map<std::string, std::string> t_loc_ext_paths;

# define MAX_METHODS 5

# define LOC_GET_INDEX 0
# define LOC_POST_INDEX 1
# define LOC_DELETE_INDEX 2
# define LOC_PUT_INDEX 3
# define LOC_HEAD_INDEX 4

class Location
{
	private:

		std::string					_path;
		std::string					_root;
		bool						_autoindex;
		std::string					_index;
		bool						_methods[5]; // GET+ POST- DELETE- PUT- HEAD-
		std::string					_return;
		std::string					_alias;
		std::vector<std::string>	_cgi_paths;
		std::vector<std::string>	_cgi_extensions;
		unsigned long				_client_max_body_size;

	public:
			// MAP OF <file extension -> extension handler file path>
		t_loc_ext_paths					_ext_paths;

		Location(void);
		Location(const Location &other);
		Location(std::string path, std::string &cntnts);
		
		Location					&operator=(const Location &other);
		bool						operator==(const Location &other);

		std::string					get_path(void);
		std::string					get_root(void);
		bool						get_autoindex(void);
		std::string					get_index(void);
		bool						get_methods(int pos);
		std::string					get_return(void);
		std::string					get_alias(void);
		std::vector<std::string>	get_cgi_paths(void);
		std::vector<std::string>	get_cgi_extensions(void);
		unsigned long				get_client_max_body_size(void);
		t_loc_ext_paths				get_ext_paths(void);

		void						set_path(std::string new_val);		
		void						set_root(std::string new_val);		
		void						set_autoindex(bool new_val);		
		void						set_index(std::string new_val);		
		void						set_methods(int pos, bool new_val);		
		void						set_return(std::string new_val);		
		void						set_alias(std::string new_val);		
		void						set_cgi_paths(std::vector<std::string> new_val);		
		void						set_cgi_extensions(std::vector<std::string> new_val);		
		void						set_client_max_body_size(unsigned long new_val);
		void						set_ext_paths(t_loc_ext_paths new_val);

		~Location();
};

#endif