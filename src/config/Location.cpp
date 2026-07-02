
#include "Location.hpp"

static size_t rev_find(char c, std::string cntnts, size_t index)
{
	for (size_t i = index; i > 0; i--)
	{
		if (cntnts[i] == c)
			return (i);
	}
	if (cntnts[0] == c)
		return (0);
	return (cntnts.npos);
}

static int	ft_stoi(std::string str)
{
	std::stringstream		ss(str);
	if (str.empty() || str.length() > 10)
		throw (std::exception());

	for (size_t i = 0; i < str.length(); ++i)
		if(!isdigit(str[i]))
			throw (std::exception());

	int						res;
	ss >> res;
	return (res);
}

std::string	Location::get_path(void)
{
	return (this->_path);
}
std::string	Location::get_root(void)
{
	return (this->_root);
}
bool	Location::get_autoindex(void)
{
	return (this->_autoindex);
}
std::string	Location::get_index(void)
{
	return (this->_index);
}
bool	Location::get_methods(int pos)
{
	return (this->_methods[pos]);
}
std::string	Location::get_return(void)
{
	return (this->_return);
}
std::string	Location::get_alias(void)
{
	return (this->_alias);
}
std::vector<std::string>	Location::get_cgi_paths(void)
{
	return (this->_cgi_paths);
}
std::vector<std::string>	Location::get_cgi_extensions(void)
{
	return (this->_cgi_extensions);
}
unsigned long	Location::get_client_max_body_size(void)
{
	return (this->_client_max_body_size);
}
t_loc_ext_paths	Location::get_ext_paths(void)
{
	return (this->_ext_paths);
}

void	Location::set_path(std::string new_val)
{
	this->_path = new_val;
}
void	Location::set_root(std::string new_val)
{
	this->_root = new_val;
}
void	Location::set_autoindex(bool new_val)
{
	this->_autoindex = new_val;
}
void	Location::set_index(std::string new_val)
{
	this->_index = new_val;
}
void	Location::set_methods(int pos, bool new_val)
{
	this->_methods[pos] = new_val;
}
void	Location::set_return(std::string new_val)
{
	this->_return = new_val;
}
void	Location::set_alias(std::string new_val)
{
	this->_alias = new_val;
}
void	Location::set_cgi_paths(std::vector<std::string> new_val)
{
	this->_cgi_paths = new_val;
}
void	Location::set_cgi_extensions(std::vector<std::string> new_val)
{
	this->_cgi_extensions = new_val;
}
void	Location::set_client_max_body_size(unsigned long new_val)
{
	this->_client_max_body_size = new_val;
}
void	Location::set_ext_paths(t_loc_ext_paths new_val)
{
	this->_ext_paths = new_val;
}

Location::Location(void)
{
	this->_path = "";
	this->_root = "";
	this->_autoindex = false;
	this->_index = "";

	this->_cgi_paths.push_back(0);
	//	BY DEFAULT, ALL LOCATIONS HAVE ONLY GET ALLOWED.
	for (size_t i = 0; i < MAX_METHODS; i++)
		this->_methods[i] = false;	// GET, POST, DELETE, PUT, HEAD
	this->_methods[0] = true;		// GET

	this->_return = "";
	this->_alias = "";
	this->_client_max_body_size = DFLT_MAX_BODY_SIZE;
}

Location::Location(const Location &other)
{
	this->_path = other._path;
	this->_root = other._root;
	this->_autoindex = other._autoindex;
	this->_index = other._index;
    for (size_t i = 0; i < MAX_METHODS; i++)
		this->_methods[i] = other._methods[i];
	this->_return = other._return;
	this->_alias = other._alias;
	this->_cgi_paths = other._cgi_paths;
	this->_cgi_extensions = other._cgi_extensions;
	this->_client_max_body_size = other._client_max_body_size;
	this->_ext_paths = other._ext_paths;
}

Location::Location(std::string path, std::string &cntnts)
{
	bool		flags[9] = {false, false, false, false, false, false, false, false, false};

	this->_path = path;
	std::cout << "CONTENT [" << cntnts << "]\n";
	while (!cntnts.empty())
	{
		size_t	index = 0;
		while (index < cntnts.size() - 1 && isspace(cntnts[index]))
			index ++;
		if (index >= cntnts.size() - 1)
			break ;
		size_t	ln_start = rev_find('\n', cntnts, index);
		if (ln_start ++ == cntnts.npos)
			ln_start = 0;

		size_t	ln_end = cntnts.find('\n', index);
		if (ln_end == cntnts.npos || ln_end == cntnts.size() - 1)
			ln_end = cntnts.size();
		if (index + 4 < cntnts.size() && !strncmp(&cntnts[index], "root", 4))
		{
			if (flags[0])
				throw (Config::ConfigBadConstrException("duplicate root"));
			index += 4;
			while (index < cntnts.size() - 1 && isspace(cntnts[index]))
				index ++;

			this->_root = cntnts.substr(index, cntnts.find(';') - index);
			flags[0] = true;
			std::cout << "\tLocation ROOT [" << this->_root << "]\n";
		}
		else if ((index + 13 < cntnts.size() && !strncmp(&cntnts[index], "allow_methods", 13))
			|| (index + 8 < cntnts.size() && !strncmp(&cntnts[index], "methods", 8)))
		{
			if (flags[1])
				throw (Config::ConfigBadConstrException("duplicate allow_methods"));

			if (cntnts[index] == 'a')
				index += 5;
			index += 8;
			while (index < cntnts.size() - 1 && isspace(cntnts[index]))
				index ++;

			std::string	extract = cntnts.substr(index, cntnts.find(';') - index);
			if (extract.empty())
				throw (Config::ConfigBadConstrException("invalid allow_methods"));
			while (!extract.empty())
			{
				while (isspace(extract[0]))
					extract.erase(0, 1);
				if (extract.empty())
					break ;
				if (!isspace(extract[extract.size() - 1]))
					extract += " ";
				if (strncmp(extract.c_str(), "GET", 3)
					&& (3 >= extract.size() || isspace(extract[3])))
				{
					this->_methods[0] = true;
					std::cout << "\t\tGET METHOD [" << extract << "]\n";
				}
				else if (strncmp(extract.c_str(), "POST", 4)
					&& (4 >= extract.size() || isspace(extract[4])))
				{
					this->_methods[1] = true;
					std::cout << "\t\tPOST METHOD [" << extract << "]\n";
				}
				else if (strncmp(extract.c_str(), "PUT", 3)
					&& (3 >= extract.size() || isspace(extract[3])))
				{
					this->_methods[2] = true;
					std::cout << "\t\tPUT METHOD [" << extract << "]\n";
				}
				else if (strncmp(extract.c_str(), "HEAD", 4)
					&& (4 >= extract.size() || isspace(extract[4])))
				{
					this->_methods[3] = true;
					std::cout << "\t\tPUT METHOD [" << extract << "]\n";
				}
				else
					throw (Config::ConfigBadConstrException("invalid allow_methods"));
				while (!isspace(extract[0]))
					extract.erase(0, 1);
				while (isspace(extract[0]))
					extract.erase(0, 1);
			}
			std::cout << "\tLocation METHODS FINISHED\n";
			flags[1] = true;
		}
		else if (index + 9 < cntnts.size() && !strncmp(&cntnts[index], "autoindex", 9))
		{
			if (flags[2])
				throw (Config::ConfigBadConstrException("duplicate autoindex"));
			index += 9;
			while (index < cntnts.size() - 1 && isspace(cntnts[index]))
				index ++;

			std::string	extract = cntnts.substr(index, cntnts.find(';') - index);
			if (strcmp(extract.c_str(), "on") == 0)
			{
				this->_autoindex = true;
				std::cout << "\tLocation AUTOINDEX [TRUE]\n";
			}
			else if (strcmp(extract.c_str(), "off") == 0)
			{
				this->_autoindex = false;
				std::cout <<"\tLocation AUTOINDEX [FALSE]\n";
			}
			else
				throw (Config::ConfigBadConstrException("invalid autoindex"));

			flags[2] = true;
		}
		else if (index + 5 < cntnts.size() && !strncmp(&cntnts[index], "index", 5))
		{
			if (flags[3])
				throw (Config::ConfigBadConstrException("duplicate index"));
			index += 5;
			while (index < cntnts.size() - 1 && isspace(cntnts[index]))
				index ++;

			this->_index = cntnts.substr(index, cntnts.find(';') - index);
			std::cout << "\tLocation INDEX [" << this->_index << "]\n";
			flags[3] = true;
		}
		else if (index + 6 < cntnts.size() && !strncmp(&cntnts[index], "return", 6))
		{
			if (flags[4])
				throw (Config::ConfigBadConstrException("duplicate return"));
			index += 6;
			while (index < cntnts.size() - 1 && isspace(cntnts[index]))
				index ++;

			this->_return = cntnts.substr(index, cntnts.find(';') - index);
			std::cout << "\tLocation RETURN [" << this->_return << "]\n";
			flags[4] = true;
		}
		else if (index + 5 < cntnts.size() && !strncmp(&cntnts[index], "alias", 5))
		{
			if (flags[5])
				throw (Config::ConfigBadConstrException("duplicate alias"));
			index += 5;
			while (index < cntnts.size() - 1 && isspace(cntnts[index]))
				index ++;

			this->_alias = cntnts.substr(index, cntnts.find(';') - index);
			std::cout << "\tLocation ALIAS [" << this->_alias << "]\n";
			flags[5] = true;
		}
		else if (index + 7 < cntnts.size() && !strncmp(&cntnts[index], "cgi_ext", 7))
		{
			if (flags[6])
				throw (Config::ConfigBadConstrException("duplicate cgi_ext"));
			index += 7;
			while (index < cntnts.size() - 1 && isspace(cntnts[index]))
				index ++;
			std::string	extract = cntnts.substr(index, cntnts.find(';') - index);
			if (extract.empty())
				throw (Config::ConfigBadConstrException("invalid cgi_ext"));
			while (!extract.empty())
			{
				while (isspace(extract[0]))
					extract.erase(0, 1);
				if (extract.empty())
					break ;
				if (!isspace(extract[extract.size() - 1]))
					extract += " ";
	
				size_t	len = 0;
				while (!isspace(extract[len]))
					len ++;
				this->_cgi_extensions.push_back(extract.substr(0, len));
				extract.erase(0, len + 1);
			}
			for (size_t i = 0; i < this->_cgi_extensions.size(); i++)
			{
				if (i == 0)
					std::cout << "\tLocation CGI_EXTENSIONS [";
				if (i != 0)
					std::cout << " | ";
				std::cout << this->_cgi_extensions[i];
				if (i == this->_cgi_extensions.size() - 1)
					std::cout << "]\n";
			}
			
			flags[6] = true;
		}
		else if (index + 8 < cntnts.size() && !strncmp(&cntnts[index], "cgi_path", 8))
		{
			if (flags[7])
				throw (Config::ConfigBadConstrException("duplicate cgi_path"));
			index += 8;
			while (index < cntnts.size() - 1 && isspace(cntnts[index]))
				index ++;
			std::string	extract = cntnts.substr(index, cntnts.find(';') - index);
			if (extract.empty())
				throw (Config::ConfigBadConstrException("invalid cgi_path"));
			while (!extract.empty())
			{
				while (isspace(extract[0]))
					extract.erase(0, 1);
				if (extract.empty())
					break ;
				if (!isspace(extract[extract.size() - 1]))
					extract += " ";
	
				size_t	len = 0;
				while (!isspace(extract[len]))
					len ++;
				this->_cgi_paths.push_back(extract.substr(0, len));
				extract.erase(0, len + 1);
			}

			for (size_t i = 0; i < this->_cgi_paths.size(); i++)
			{
				if (i == 0)
					std::cout << "\tLocation CGI_PATHS [";
				if (i != 0)
					std::cout << " | ";
				std::cout << this->_cgi_paths[i];
				if (i == this->_cgi_paths.size() - 1)
					std::cout << "]\n";
			}
			flags[7] = true;
		}
		else if (index + 20 < cntnts.size() && !strncmp(&cntnts[index], "client_max_body_size", 20))
		{
			if (flags[8])
				throw (Config::ConfigBadConstrException("duplicate client_max_body_size"));
			index += 20;
			while (index < cntnts.size() - 1 && isspace(cntnts[index]))
				index ++;
			this->_client_max_body_size = ft_stoi(cntnts.substr(index, cntnts.find(';') - index));
			std::cout << "\tLocation CLIENT_MAX_BODY_SIZE [" << this->_client_max_body_size << "]\n";
			flags[8] = true;
		}
		else
			throw (Config::ConfigBadConstrException("invalid entry"));
		cntnts.erase(ln_start, (ln_end - ln_start) + 1);
	}
}

Location &Location::operator=(const Location &other)
{
	if (this != &other)
	{
		this->_path = other._path;
		this->_root = other._root;
		this->_autoindex = other._autoindex;
		this->_index = other._index;
		for (size_t i = 0; i < MAX_METHODS; i++)
			this->_methods[i] = other._methods[i];
		this->_return = other._return;
		this->_alias = other._alias;
		this->_cgi_paths = other._cgi_paths;
		this->_cgi_extensions = other._cgi_extensions;
		this->_client_max_body_size = other._client_max_body_size;
		this->_ext_paths = other._ext_paths;
	}
	return (*this);
}

bool	Location::operator==(const Location &other)
{
	if (this->_path != other._path || this->_root != other._root
		|| this->_autoindex != other._autoindex || this->_index != other._index
		|| this->_return != other._return || this->_alias != other._alias
		|| this->_client_max_body_size != other._client_max_body_size
		|| this->_ext_paths != other._ext_paths)
		return (false);
	
	for (size_t i = 0; i < MAX_METHODS; i++)
		if (this->_methods[i] != other._methods[i])
			return (false);

	for (size_t i = 0; i < _cgi_paths.size(); i++)
		if (this->_cgi_paths[i] != other._cgi_paths[i])
			return (false);

	for (size_t i = 0; i < _cgi_extensions.size(); i++)
		if (this->_cgi_extensions[i] != other._cgi_extensions[i])
			return (false);

	return (true);
}

Location::~Location()
{
}
