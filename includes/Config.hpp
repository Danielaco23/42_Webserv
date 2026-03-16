
#pragma once

#include <string>	// HTTP petition handler
#include <vector>	// Client list
#include <map>		// HTTP headers

class Config
{
	private:
		std::string		_file_Path;		// Path of the config file.

		int				_listen_Port;	// Listening port, mandatory.
		std::string		_host_Ip;		// Host or 127.0.0.1 by default.
		std::string		_server_Name;
		int				_error_Code;	// Default error code.
		std::string		_error_Page;	// Default error page directory.

	public:
		Config();

};
