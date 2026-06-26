
#include "Config.hpp"

#define FILE_READ_BUFFER 256

static int	ft_stoi(std::string str)
{
	std::stringstream		ss(str);
	if (str.length() > 10)
		throw (std::exception());

	for (size_t i = 0; i < str.length(); ++i)
		if(!isdigit(str[i]))
			throw (std::exception());

	int						res;
	ss >> res;
	return (res);
}

static void	buff_reset(char *str)
{
	for (size_t i = 0; i < FILE_READ_BUFFER; i++)
		str[i] = '\0';
}

static std::string	read_whole_file(std::string file)
{
	std::ifstream	input;
	char			buffer[FILE_READ_BUFFER];
	std::string		cntnt;

	input.open(file.c_str());

	if (!input.is_open())
		return (std::cerr << "Failed to open file\n", "");
	buffer[0] = 'a';
	while (buffer[0])
	{
		buff_reset(buffer);
		input.read(buffer, (FILE_READ_BUFFER - 1));
		cntnt = cntnt + buffer;
	}
	input.close();
	size_t pos = 0;
	while ((pos = cntnt.find("\n\n")) != cntnt.npos)
		cntnt.erase(pos, 1);
	return (cntnt);
}

static std::string	extract_from_brackets(std::string cntnt)
{
	size_t			start = cntnt.find('{');
	while (start < cntnt.size() - 3 && isspace(cntnt[start + 2]))
		start ++;
	size_t			possible_end = cntnt.find('}');
	size_t			cp = start;

	if (possible_end == cntnt.npos || start == cntnt.npos)
		return ("");
	while (possible_end != cntnt.npos && cp < possible_end)
	{
		cp = cntnt.find('{', possible_end + 1);
		possible_end = cntnt.find('}', possible_end + 1);
	}
	std::string		result = cntnt.substr(start + 1, (possible_end - 1) - (start + 1));
	if (result[result.size() - 1] == '\n')
		result.erase(result.size() - 1);
	return (result);
}

static int	server_count(bool add_one)
{
	static int	n_servs = 0;

	if (add_one)
		n_servs ++;
	return (n_servs);
}

static std::string	extract_server_info(std::string cntnt)
{
	size_t			n_servs = server_count(false);
	size_t			s_pos = cntnt.find("\nserver");

	if (s_pos == cntnt.npos
			|| (cntnt.size() > 6 && cntnt.compare(0, 6, "server") == 0 && isspace(cntnt[6])))
		s_pos = 0;
	for (size_t i = 0; i < n_servs; i++)
		if ((s_pos = cntnt.find("\nserver", s_pos + 7)) == cntnt.npos)
			return ("");
	return (server_count(true), extract_from_brackets(cntnt.substr(s_pos)));
}

/* static size_t	count_lines(std::string cntnt)
{
	size_t			lines = 0;
	size_t			n = 0;
	while (n != cntnt.npos)
	{
		n = cntnt.find('\n', n + 1);
		if (n == cntnt.size() - 1)
			break ;
		lines ++;
	}
	return (lines);
} */

static void	rmv_char_onwards(std::string &contents, std::string dth_mrk)
{
	size_t	start_index = contents.find(dth_mrk);	// Find the first occurrence, if any.
	
	while (start_index != contents.npos)
	{
		size_t	end_index = contents.find('\n', start_index);	// Find the end of the line.
		while (start_index > 0	// And the rest of the spaces before it, while you're at it.
			&& (std::isspace(contents[start_index - 1]) || contents.substr(start_index - 1, dth_mrk.size()) == dth_mrk))
			start_index -= dth_mrk.size();
		contents.erase(start_index, end_index - start_index);	// Nuke it all.
		start_index = contents.find(dth_mrk);					// Next target.
	}		// If no occurrence found, loop is not entered.
}

Config::Config(void): _port(8080), _root("/"), _index("index.html")
{
	_host[0] = 127;
	_host[1] = 0;
	_host[2] = 0;
	_host[3] = 1;
}

static size_t rev_find(char c, std::string cntnt, size_t index)
{
	for (size_t i = index; i > 0; i--)
	{
		if (cntnt[i] == c)
			return (i);
	}
	if (cntnt[0] == c)
		return (0);
	return (cntnt.npos);
}

void	Config::parse_port(std::string &cntnt)
{
	std::string		word = "listen";
	size_t			index = cntnt.find(word);
	size_t			start = rev_find('\n', cntnt, index);
	size_t			end = cntnt.find('\n', index);

	if (start == cntnt.npos)
		start = 0;
	else
		start ++;
	while (this->_port == 0)
	{
		if (index == cntnt.npos)
			throw (std::cerr << "Config file missing required listen entry for server [" << server_count(false) << "].\n", ConfigBadConstructException());
		if (isspace(cntnt[index + word.length()]) && (rev_find('{', cntnt, index) == cntnt.npos
			|| rev_find('}', cntnt, index) != cntnt.npos))
		{
			index += word.length();
			while (isspace(cntnt[index]))
				index ++;
			if (isdigit(cntnt[index]) && isdigit(cntnt[index + 1]) && isdigit(cntnt[index + 2]) && isdigit(cntnt[index + 3]))
				this->_port = ft_stoi(cntnt.substr(index, 4));
			/*else if (isdigit(cntnt[index]) && isdigit(cntnt[index + 1]))
				this->_port = ft_stoi(cntnt.substr(index, 2));*/
			else
				throw (std::cerr << "Invalid port number for server [" << server_count(false) << "].\n", ConfigBadConstructException());
		}
		else if (index != cntnt.size() - 1)
			index = cntnt.find(word, index + 1);
		else
			throw (std::cerr << "Config file missing required listen port for server [" << server_count(false) << "].\n", ConfigBadConstructException());
	}
	cntnt.erase(start, end - start + 1);
	std::cout << "PORT CONFIRMED [" << this->_port << "]\n";
}

void	Config::parse_host(std::string &cntnt)
{
	std::string		word = "host";
	size_t			index = cntnt.find(word);
	size_t			start = rev_find('\n', cntnt, index);
	size_t			end = cntnt.find('\n', index);

	if (start == cntnt.npos)
		start = 0;
	else
		start ++;
	while (this->_host[0] == 0)
	{
		if (index == cntnt.npos)
			throw (std::cerr << "Config file missing required host entry for server [" << server_count(false) << "].\n", ConfigBadConstructException());
		if (isspace(cntnt[index + word.length()]) && (cntnt.find("localhost") == cntnt.npos
			|| index - cntnt.find("localhost") != 5) && (rev_find('{', cntnt, index) == cntnt.npos
			|| rev_find('}', cntnt, index) != cntnt.npos))
		{
			index += word.length();
			while (isspace(cntnt[index]))
				index ++;
			try
			{
				std::string host_num = cntnt.substr(index, cntnt.find(';', index) - index);
				for (size_t i = 0; i < 4; i++)
				{
					size_t	end = host_num.find('.');
					if (end == host_num.npos)
						end = host_num.size();
					this->_host[i] = ft_stoi(host_num.substr(0, end));
					while (host_num[0] && host_num[0] != '.')
						host_num.erase(0, 1);
					if (host_num[0] == '.')
						host_num.erase(0, 1);
				}
			}
			catch(const std::exception& e)
			{
				throw (std::cerr << e.what() << "\nInvalid host for server [" << server_count(false) << "].\n", ConfigBadConstructException());
			}
		}
		else if (index != cntnt.size() - 1)
			index = cntnt.find(word, index + 1);
		else
			throw (std::cerr << "Config file missing required host for server [" << server_count(false) << "].\n", ConfigBadConstructException());
	}
	cntnt.erase(start, end - start + 1);
	std::cout << "HOST CONFIRMED [" << this->_host[0] << "." << this->_host[1] << "." << this->_host[2] << "." << this->_host[3] << "]\n";
}

void	Config::parse_root(std::string &cntnt)
{
	std::string		word = "root";
	size_t			index = cntnt.find(word);
	size_t			start = rev_find('\n', cntnt, index);
	size_t			end = cntnt.find('\n', index);

	if (start == cntnt.npos)
		start = 0;
	else
		start ++;
	while (this->_root.empty())
	{
		if (index == cntnt.npos)
			throw (std::cerr << "Config file missing required root entry for server [" << server_count(false) << "].\n", ConfigBadConstructException());
		if (isspace(cntnt[index + word.length()]) && (rev_find('{', cntnt, index) == cntnt.npos
			|| rev_find('}', cntnt, index) != cntnt.npos))
		{
			index += word.length();
			while (isspace(cntnt[index]))
				index ++;
			this->_root = cntnt.substr(index, cntnt.find(';', index) - index);
		}
		else if (index != cntnt.size() - 1)
			index = cntnt.find(word, index + 1);
		else
			throw (std::cerr << "Config file missing required root for server [" << server_count(false) << "].\n", ConfigBadConstructException());
	}
	cntnt.erase(start, end - start + 1);
	std::cout << "ROOT CONFIRMED [" << this->_root << "]\n";
}

void	Config::parse_indx(std::string &cntnt)
{
	std::string		word = "index";
	size_t			index = cntnt.find(word);
	size_t			start = rev_find('\n', cntnt, index);
	size_t			end = cntnt.find('\n', index);

	if (start == cntnt.npos)
		start = 0;
	else
		start ++;
	while (this->_index.empty())
	{
		if (index == cntnt.npos)
			throw (std::cerr << "Config file missing required index entry for server [" << server_count(false) << "].\n", ConfigBadConstructException());
		if (isspace(cntnt[index + word.length()]) && (rev_find('{', cntnt, index) == cntnt.npos
			|| rev_find('}', cntnt, index) != cntnt.npos))
		{
			index += word.length();
			while (isspace(cntnt[index]))
				index ++;
			this->_index = cntnt.substr(index, cntnt.find(';', index) - index);
		}
		else if (index != cntnt.size() - 1)
			index = cntnt.find(word, index + 1);
		else
			throw (std::cerr << "Config file missing required index for server [" << server_count(false) << "].\n", ConfigBadConstructException());
	}
	cntnt.erase(start, end - start + 1);
	std::cout << "INDX CONFIRMED [" << this->_index << "]\n";
}

/*
_server_name
_client_max_body_size
_autoindex
_error_pages
_locations
_server_address
_listen_fd
*/
void	Config::parse_optional(std::string &cntnt)
{
	(void) cntnt;
	(void) _client_max_body_size;
	(void) _autoindex;
	(void) _listen_fd;
}

Config::Config(std::string config_file): _is_real(false), _port(0), _root(""), _index(""), _client_max_body_size(0), _autoindex(false), _listen_fd(-1)
{
	for (size_t i = 0; i < 4; i++)
		this->_host[i] = 0;
	if (config_file.empty() || config_file.size() < 6)
		throw (std::cerr << "Config file name too small\n", ConfigBadConstructException());
	if (config_file.substr(config_file.size() - 5) != ".conf")
		throw (std::cerr << "Config file wrong file extension.\n", ConfigBadConstructException());

	std::string		cntnt = read_whole_file(config_file);
	rmv_char_onwards(cntnt, "#");
	if (cntnt.empty())
		throw (std::cerr << "Config file is empty.\n", ConfigBadConstructException());
	cntnt = extract_server_info(cntnt);
	if (cntnt.empty())
		throw (std::cerr << "Config file wrong format.\n", ConfigBadConstructException());

	parse_port(cntnt);  // REQUIRED PARAMETERS.
	parse_host(cntnt);  // REQUIRED PARAMETERS.
	parse_root(cntnt);  // REQUIRED PARAMETERS.
	parse_indx(cntnt);  // REQUIRED PARAMETERS.
	std::cout << "CONTENT [\n" << cntnt << "]\n";
	this->_is_real = true;
	// std::cout << this->_port << std::endl;
}

const char	*Config::ConfigBadConstructException::what() const throw()
{
	return ("Could not construct config class.");
}

int main(int argc, char *argv[])
{
	for (int index = 1; index < argc; index++)
	{
		std::cout << std::endl << "-----------------------------------\n" << std::endl;
		try
		{
			std::cout << "FILE [" << argv[index] << "]:\n";
			Config	configFile(argv[index]);
		}
		catch(const std::exception& e)
		{
			//std::cerr << e.what() << std::endl;
		}
	}
	return (0);
}

Config::~Config()
{}
