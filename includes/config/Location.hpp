#pragma once

#ifndef LOCATION_HPP
# define LOCATION_HPP

# include "Webserver.hpp"

class Location
{
	private:
		/* std::string					_path;
		std::string					_root;
		bool						_autoindex;
		std::string					_index;
		std::vector<short>			_methods; // GET+ POST- DELETE- PUT- HEAD-
		std::string					_return;
		std::string					_alias;
		std::vector<std::string>	_cgi_path;
		std::vector<std::string>	_cgi_ext;
		unsigned long				_client_max_body_size; */
	public:
		Location(void);
		~Location();
};

Location::Location(/* args */)
{
}

Location::~Location()
{
}


#endif