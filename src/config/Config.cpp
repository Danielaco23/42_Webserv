
#include "Config.hpp"

#define FILE_READ_BUFFER 256

template <typename T>
static std::string toString(const T val)
{
	std::stringstream stream;
	stream << val;
	return stream.str();
}

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

static int	server_count(bool add_one, bool reset)
{
	static int	n_servs = 0;
	if (add_one)
		n_servs ++;
	int			ret = n_servs;
	if (reset)
		n_servs = 0;
	return (ret);
}

void	Config::throw_with_msg(std::string msg)
{
	std::string err_msg = msg + " for server number [" + toString(server_count(false, false)) + "]";
	throw (ConfigBadConstrException(err_msg));
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
	std::string		cntnts;

	input.open(file.c_str());
	if (!input.is_open())
		throw (Config::ConfigBadConstrException("Failed to open file"));

	buffer[0] = 'a';
	while (buffer[0])
	{
		buff_reset(buffer);
		input.read(buffer, (FILE_READ_BUFFER - 1));
		cntnts = cntnts + buffer;
	}
	input.close();
	size_t			pos = 0;
	while ((pos = cntnts.find("\n\n")) != cntnts.npos)
		cntnts.erase(pos, 1);
	while (isspace(cntnts[cntnts.size() - 1]))
		cntnts.erase(cntnts.size() - 1, 1);
	return (cntnts);
}

static std::string	extract_from_brackets(std::string cntnts)
{
	size_t			start = cntnts.find('{');
	while (start < cntnts.size() - 3 && isspace(cntnts[start + 2]))
		start ++;
	size_t			possible_end = cntnts.find('}');
	size_t			cp = cntnts.find('{', start + 1);

	if (possible_end == cntnts.npos || start == cntnts.npos)
		throw (Config::ConfigBadConstrException("Wrong \"{}\" formatting for server entry."));
	while (cp != cntnts.npos && possible_end != cntnts.npos && cp < possible_end)
	{
		cp = cntnts.find('{', possible_end + 1);
		possible_end = cntnts.find('}', possible_end + 1);
	}
	while (possible_end != cntnts.npos && possible_end >= 2 && isspace(cntnts[possible_end - 2]))
		possible_end--;
	
	std::string		result = cntnts.substr(start + 1, (possible_end - 1) - (start + 1));
	if (result[result.size() - 1] == '\n')
		result.erase(result.size() - 1);
	return (result);
}

static std::string	extract_server_info(std::string cntnts)
{
	size_t			n_servs = server_count(false, false);
	size_t			s_pos = cntnts.find("\nserver");

	if (s_pos == cntnts.npos
			|| (cntnts.size() > 6 && cntnts.compare(0, 6, "server") == 0 && isspace(cntnts[6])))
		s_pos = 0;
	for (size_t i = 0; i < n_servs; i++)
		if ((s_pos = cntnts.find("\nserver", s_pos + 7)) == cntnts.npos)
			throw (Config::ConfigBadConstrException("No more servers to read"));
	return (extract_from_brackets(cntnts.substr(s_pos)));
}

/* static size_t	count_lines(std::string cntnts)
{
	size_t			lines = 0;
	size_t			n = 0;
	while (n != cntnts.npos)
	{
		n = cntnts.find('\n', n + 1);
		if (n == cntnts.size() - 1)
			break ;
		lines ++;
	}
	return (lines);
} */

static void	rmv_all_char_onwards(std::string &cntnts, std::string dth_mrk)
{
	size_t	start_index = cntnts.find(dth_mrk);	// Find the first occurrence, if any.
	
	while (start_index != cntnts.npos)
	{
		size_t	end_index = cntnts.find('\n', start_index);	// Find the end of the line.
		while (start_index > 0	// And the rest of the spaces before it, while you're at it.
			&& (std::isspace(cntnts[start_index - 1]) || cntnts.substr(start_index - 1, dth_mrk.size()) == dth_mrk))
			start_index -= dth_mrk.size();
		cntnts.erase(start_index, end_index - start_index);	// Nuke it all.
		start_index = cntnts.find(dth_mrk);					// Next target.
	}		// If no occurrence found, loop is not entered.
}

static void check_spacing(std::string &cntnts)
{
	std::string		indent_unit = "";
	size_t			level = 0;
	size_t			curr_ln_start = 0;

	while (!cntnts.empty() && curr_ln_start <= cntnts.size() - 1)
	{
		std::string		curr_ln;
		size_t			curr_ln_end = cntnts.find('\n', curr_ln_start);

		if (curr_ln_end == cntnts.npos)
			curr_ln_end = cntnts.size() - 1;

		//	TRUNCATE TRAILING WHITESPACES IN ALL LINES
		while (curr_ln_end > 0 && isspace(cntnts[curr_ln_end - 1]) && cntnts[curr_ln_end - 1] != '\n')
		{
			if (curr_ln_end != cntnts.size() - 1)
				cntnts.erase(--curr_ln_end, 1);
			else
				cntnts.erase(curr_ln_end--, 1);
		}
		if (curr_ln_end == cntnts.size() - 1 && isspace(cntnts[curr_ln_end]) && cntnts[curr_ln_end] != '\n')
			cntnts.erase(curr_ln_end--, 1);

		curr_ln = cntnts.substr(curr_ln_start, (curr_ln_end - curr_ln_start) + 1);
		if (curr_ln.find('}') != curr_ln.npos && curr_ln.find('{') == curr_ln.npos)
		{
			if (level == 0)
				throw (Config::ConfigBadConstrException("INCORRECT USE OF \"{}\" IN FILE"));
			level --;
		}
		if (level > 0)
		{
			if (indent_unit.empty())
			{
				for (size_t i = 0; isspace(curr_ln[i]); i++)
					indent_unit += curr_ln[i++];
			}
			for (size_t i = 0; i < level; i++)
			{
				size_t		offset = i * (indent_unit.size() - 1);
				if (strncmp(indent_unit.c_str(), curr_ln.c_str() + offset, indent_unit.size() - 1) != 0)
					throw (Config::ConfigBadConstrException("INCORRECT/INCONSISTENT INDENTATION"));
			}
		}
		else
			if (curr_ln.size() > 1 && isspace(curr_ln[0]))
				throw (Config::ConfigBadConstrException("INCORRECT/INCONSISTENT INDENTATION"));
	
		if (curr_ln.find('{') != curr_ln.npos && curr_ln.find('}') == curr_ln.npos)
			level ++;
		curr_ln_start = curr_ln_end + 1;
	}
	if (level > 0)
		throw (std::cerr << level << "\t", Config::ConfigBadConstrException("OPEN \"{\" LEFT UNCLOSED IN FILE"));
	
	while ((level = cntnts.find("\n\n")) != cntnts.npos)
		cntnts.erase(level, 1);
}

static void check_boundaries(std::string &cntnts)
{
	size_t			curr_ln_start = 0;

	while (!cntnts.empty() && curr_ln_start < cntnts.size() - 1)
	{
		size_t			curr_ln_end = cntnts.find('\n', curr_ln_start);

		if (curr_ln_end == cntnts.npos)
			curr_ln_end = cntnts.size() - 1;
		if (curr_ln_end > 0 && cntnts[curr_ln_end - 1] != '{' && cntnts[curr_ln_end - 1] != '}'
			&& cntnts[curr_ln_end - 1] != ';' && cntnts[curr_ln_end - 1] != '\n')
			throw (Config::ConfigBadConstrException("Unauthorized line termination found in config file"));
		curr_ln_start = curr_ln_end + 1;
	}
}

Config::Config(void):_is_real(false), _port(0), _www_root(""), _index(DFLT_INDEX), _server_name(DFLT_S_NAME), _client_max_body_size(DFLT_MAX_BODY_SIZE), _autoindex(DFLT_AUTOINDEX), _server_address(DFLT_ADDRESS), _listen_fd(DFLT_LISTEN_FD)
{
	this->_host[0] = DFLT_HOST_0;
	this->_host[1] = DFLT_HOST_1;
	this->_host[2] = DFLT_HOST_2;
	this->_host[3] = DFLT_HOST_3;
}

void	Config::parse_required(std::string &cntnts)
{
	parse_port(cntnts);  // REQUIRED PARAMETERS.
	parse_root(cntnts);  // REQUIRED PARAMETERS.
}

void	Config::parse_port(std::string &cntnts)
{
	std::string		word = "listen";
	size_t			index = cntnts.find(word);

	while (this->_port == 0)
	{
		if (index == cntnts.npos)
			throw_with_msg("Config file missing required listen entry");
		if (isspace(cntnts[index + word.length()]) && (rev_find('{', cntnts, index) == cntnts.npos
			|| rev_find('}', cntnts, index) != cntnts.npos))
		{
			size_t			start = rev_find('\n', cntnts, index);
			size_t			end = cntnts.find('\n', index);

			if (end == cntnts.npos)
				end = cntnts.find(';', index);

			if (start == cntnts.npos)
				start = 0;
			else
				start ++;
			index += word.length();
			while (isspace(cntnts[index]))
				index ++;
			if (isdigit(cntnts[index]) && isdigit(cntnts[index + 1]) && isdigit(cntnts[index + 2]) && isdigit(cntnts[index + 3]))
			{
				this->_port = ft_stoi(cntnts.substr(index, 4));
				cntnts.erase(start, end - start + 1);
			}	
			/*else if (isdigit(cntnts[index]) && isdigit(cntnts[index + 1]))
				this->_port = ft_stoi(cntnts.substr(index, 2));*/
			else
				throw_with_msg("Invalid port number");
		}
		else if (index != cntnts.size() - 1)
			index = cntnts.find(word, index + 1);
		else
			throw_with_msg("Config file missing required listen port");
	}
	// std::cout << "PORT CONFIRMED [" << this->_port << "]\n";
}

void	Config::parse_root(std::string &cntnts)
{
	std::string		word = "root";
	size_t			index = cntnts.find(word);

	while (this->_www_root.empty())
	{
		if (index != cntnts.npos && isspace(cntnts[index + word.length()])
			&& (rev_find('{', cntnts, index) == cntnts.npos || rev_find('}', cntnts, index) != cntnts.npos))
		{
			size_t			start = rev_find('\n', cntnts, index);
			size_t			end = cntnts.find('\n', index);

			if (end == cntnts.npos)
				end = cntnts.find(';', index);

			if (start == cntnts.npos)
				start = 0;
			else
				start ++;

			index += word.length();
			while (isspace(cntnts[index]))
				index ++;
			this->_www_root = cntnts.substr(index, cntnts.find(';', index) - index);
			cntnts.erase(start, end - start + 1);
		}
		else if (index != cntnts.npos && index != cntnts.size() - 1)
			index = cntnts.find(word, index + 1);
		else
			throw_with_msg("Config file missing required root entry");
		}
	// std::cout << "ROOT CONFIRMED [" << this->_www_root << "]\n";
}

void	Config::parse_optional(std::string &cntnts)
{
	parse_host(cntnts);				// APPARENTLY defaults to 127.0.0.1 (DFLT_HOST_[0-3]) ?
	parse_index(cntnts);			// Defaults to DFLT_INDEX ("index.html")
	parse_s_name(cntnts);
	parse_cmbs(cntnts);
	parse_autoindex(cntnts);
	parse_error_pages(cntnts);
	parse_locations(cntnts);
	parse_s_address(cntnts);
}

void	Config::parse_host(std::string &cntnts)
{
	std::string		word = "host";
	size_t			index = cntnts.find(word);

	while (this->_host[0] == 0)
	{
		if (index != cntnts.npos && isspace(cntnts[index + word.length()])
			&& (cntnts.find("localhost") == cntnts.npos || index - cntnts.find("localhost") != 5)
			&& (rev_find('{', cntnts, index) == cntnts.npos || rev_find('}', cntnts, index) != cntnts.npos))
		{
			size_t		start = rev_find('\n', cntnts, index);
			size_t		end = cntnts.find('\n', index);

			if (end == cntnts.npos)
				end = cntnts.find(';', index);

			if (start == cntnts.npos)
				start = 0;
			else
				start ++;
			index += word.length();
			while (isspace(cntnts[index]))
				index ++;
			try
			{
				std::string host_num = cntnts.substr(index, cntnts.find(';', index) - index);
				if (host_num != "localhost")
				{
					for (size_t i = 0; i < 4; i++)
					{
						size_t	num_end = host_num.find('.');
						if (i == 3 || num_end == host_num.npos)
						{
							if (i != 3)
								throw (std::exception());
							num_end = host_num.size();
						}
						this->_host[i] = ft_stoi(host_num.substr(0, num_end));
						while (host_num[0] && host_num[0] != '.')
							host_num.erase(0, 1);
						if (host_num[0] == '.')
							host_num.erase(0, 1);
					}
				}
				else
					index = cntnts.size() - 1;
				cntnts.erase(start, end - start + 1);
			}
			catch(const std::exception& e)
			{
				throw_with_msg("Invalid host number");
			}
		}
		if (index != cntnts.npos && index != cntnts.size() - 1)
			index = cntnts.find(word, index + 1);
		else
		{	// DEFAULTS TO 127.0.0.1
			this->_host[0] = DFLT_HOST_0;
			this->_host[1] = DFLT_HOST_1;
			this->_host[2] = DFLT_HOST_2;
			this->_host[3] = DFLT_HOST_3;
		}
	}
	// std::cout << "HOST CONFIRMED [" << this->_host[0] << "." << this->_host[1] << "." << this->_host[2] << "." << this->_host[3] << "]\n";
}

void	Config::parse_index(std::string &cntnts)
{
	std::string		word = "index";
	size_t			index = cntnts.find(word);

	while (this->_index.empty())
	{
		if (index != cntnts.npos && isspace(cntnts[index + word.length()])
			&& (rev_find('{', cntnts, index) == cntnts.npos || rev_find('}', cntnts, index) != cntnts.npos))
		{
			size_t			start = rev_find('\n', cntnts, index);
			size_t			end = cntnts.find('\n', index);

			if (end == cntnts.npos)
				end = cntnts.find(';', index);

			if (start == cntnts.npos)
				start = 0;
			else
				start ++;

			index += word.length();
			while (isspace(cntnts[index]))
				index ++;
			this->_index = cntnts.substr(index, cntnts.find(';', index) - index);
			cntnts.erase(start, end - start + 1);
		}
		else if (index != cntnts.npos && index != cntnts.size() - 1)
			index = cntnts.find(word, index + 1);
		else
			this->_index = DFLT_INDEX;
	}
	// std::cout << "INDEX CONFIRMED [" << this->_index << "]\n";
}

void	Config::parse_s_name(std::string &cntnts)
{
	std::string		word = "server_name";
	size_t			index = cntnts.find(word);

	while (this->_server_name.empty())
	{
		if (index != cntnts.npos && isspace(cntnts[index + word.length()])
			&& (rev_find('{', cntnts, index) == cntnts.npos || rev_find('}', cntnts, index) != cntnts.npos))
		{
			size_t			start = rev_find('\n', cntnts, index);
			size_t			end = cntnts.find('\n', index);

			if (end == cntnts.npos)
				end = cntnts.find(';', index);

			if (start == cntnts.npos)
				start = 0;
			else
				start ++;
			index += word.length();
			while (isspace(cntnts[index]))
				index ++;
			this->_server_name = cntnts.substr(index, cntnts.find(';', index) - index);
			cntnts.erase(start, end - start + 1);
		}
		else if (index != cntnts.npos && index != cntnts.size() - 1)
			index = cntnts.find(word, index + 1);
		else
			this->_server_name = DFLT_S_NAME;
	}
	// std::cout << "SERVER_NAME CONFIRMED [" << this->_server_name << "]\n";
}

void	Config::parse_cmbs(std::string &cntnts)
{
	std::string		word = "client_max_body_size";
	size_t			index = cntnts.find(word);

	while (true)
	{
		if (index != cntnts.npos && isspace(cntnts[index + word.length()])
			&& (rev_find('{', cntnts, index) == cntnts.npos || rev_find('}', cntnts, index) != cntnts.npos))
		{
			size_t			start = rev_find('\n', cntnts, index);
			size_t			end = cntnts.find('\n', index);

			if (end == cntnts.npos)
				end = cntnts.find(';', index);

			if (start == cntnts.npos)
				start = 0;
			else
				start ++;
			index += word.length();
			while (isspace(cntnts[index]))
				index ++;
			this->_client_max_body_size = ft_stoi(cntnts.substr(index, cntnts.find(';', index) - index));
			cntnts.erase(start, end - start + 1);
			break ;
		}
		else if (index != cntnts.npos && index != cntnts.size() - 1)
			index = cntnts.find(word, index + 1);
		else
		{
			this->_client_max_body_size = DFLT_MAX_BODY_SIZE;
			break ;
		}
	}
	// std::cout << "CLIENT MAX BODY SIZE CONFIRMED [" << this->_client_max_body_size << "]\n";
}

void	Config::parse_autoindex(std::string &cntnts)
{
	std::string		word = "autoindex";
	size_t			index = cntnts.find(word);

	while (true)
	{
		if (index != cntnts.npos && isspace(cntnts[index + word.length()])
			&& (rev_find('{', cntnts, index) == cntnts.npos || rev_find('}', cntnts, index) != cntnts.npos))
		{
			size_t			start = rev_find('\n', cntnts, index);
			size_t			end = cntnts.find('\n', index);

			if (end == cntnts.npos)
				end = cntnts.find(';', index);

			if (start == cntnts.npos)
				start = 0;
			else
				start ++;
			index += word.length();
			while (isspace(cntnts[index]))
				index ++;
			std::string	res = cntnts.substr(index, cntnts.find(';', index) - index);
			if (strcmp(res.c_str(), "on"))
				this->_autoindex = true;
			else
				this->_autoindex = false;
			cntnts.erase(start, end - start + 1);
			break ;
		}
		else if (index != cntnts.npos && index != cntnts.size() - 1)
			index = cntnts.find(word, index + 1);
		else
		{
			this->_autoindex = DFLT_S_NAME;
			break ;
		}
	}
	// std::cout << "AUTOINDEX CONFIRMED [";
	// if (this->_autoindex)
		// std::cout << "TRUE]\n";
	// else
		// std::cout << "FALSE]\n";
}

void	Config::parse_error_pages(std::string &cntnts)
{
	std::string		word = "error_page";
	size_t			index = cntnts.find(word);

	while (index != cntnts.npos)
	{
		while (true)
		{
			if (index != cntnts.npos && isspace(cntnts[index + word.length()])
				&& (rev_find('{', cntnts, index) == cntnts.npos || rev_find('}', cntnts, index) != cntnts.npos))
			{
				size_t			start = rev_find('\n', cntnts, index);
				size_t			end = cntnts.find('\n', index);

				if (end == cntnts.npos)
					end = cntnts.find(';', index);

				if (start == cntnts.npos)
					start = 0;
				else
					start ++;
				index += word.length();
				while (isspace(cntnts[index]))
					index ++;

				size_t	arg_start = index;

				while (!isspace(cntnts[index]))
					index ++;

				short	entry_num = ft_stoi(cntnts.substr(arg_start, index - arg_start));

				if (!this->_error_pages[entry_num].empty())
					throw_with_msg("Duplicate error pages present");

				while (isspace(cntnts[index]))
					index ++;

				size_t	arg_end = cntnts.find(';', index);
				this->_error_pages[entry_num] = cntnts.substr(index, arg_end - index);
				// std::cout << "ERROR_PAGE NUM " << entry_num << " CONFIRMED [" << this->_error_pages[entry_num] << "]\n";
				cntnts.erase(start, end - start + 1);
				break ;
			}
			else if (index != cntnts.npos && index != cntnts.size() - 1)
				index = cntnts.find(word, index + 1);
			else
			{
				this->_error_pages[DFLT_ERRPAGE_NUM] = DFLT_ERRPAGE_ADD;
				word = "DEFAULT";
				break ;
			}
		}
		if (word == "DEFAULT")
			break ;
		index = cntnts.find(word);
	}
	// std::cout << "ERROR_PAGES FINISHED\n";
}

void	Config::parse_locations(std::string &cntnts)
{
	std::string					word = "location";
	size_t						index = cntnts.find(word);

	while (index != cntnts.npos)
	{
		while (true)
		{
			if (index != cntnts.npos && isspace(cntnts[index + word.length()])
			&& (rev_find('{', cntnts, index) == cntnts.npos || rev_find('}', cntnts, index) != cntnts.npos))
			{
				size_t			start = rev_find('\n', cntnts, index);
				size_t			end = cntnts.find('}', index);

				if (end == cntnts.npos)
					throw_with_msg("Location left unclosed");
				if (end < cntnts.size() - 1 && cntnts[end + 1] == '\n')
					end ++;

				if (start == cntnts.npos)
					start = 0;
				else
					start ++;

				index += word.length();
				while (isspace(cntnts[index]))
					index ++;

				size_t	arg_start = index;
				while (!isspace(cntnts[index]))
					index ++;
				std::string		entry_name = cntnts.substr(arg_start, index - arg_start);
				while (isspace(cntnts[index]))
					index ++;
				std::string		entry_cnt = extract_from_brackets(cntnts);
				this->add_location(entry_name, entry_cnt);
				cntnts.erase(start, end - start + 1);
				break ;
			}
			else if (index != cntnts.npos && index != cntnts.size() - 1)
				index = cntnts.find(word, index + 1);
			else
			{
				word = "DEFAULT";
				// std::cout << "LOCATIONS FINISHED WITH DEFAULT OUTPUT\n";
				return ;
			}
		}
		index = cntnts.find(word);
	}
	// std::cout << "LOCATIONS FINISHED\n";
}

void	Config::parse_s_address(std::string &cntnts)
{
	(void)cntnts;
}

/*
	---MAIN CONFIG FUNCTION---
*/
Config::Config(std::string config_file): _is_real(false), _port(0), _www_root(""), _index(""), _client_max_body_size(0), _autoindex(false), _listen_fd(-1)
{
	for (size_t i = 0; i < 4; i++)
		this->_host[i] = 0;
	if (config_file.empty() || config_file.size() < 6)
		throw (ConfigBadConstrException("Config file name too small"));
	if (config_file.substr(config_file.size() - 5) != ".conf")
		throw (ConfigBadConstrException("Config file wrong file extension"));

	std::string		cntnts = read_whole_file(config_file);
	rmv_all_char_onwards(cntnts, "#");
	check_spacing(cntnts);
	if (cntnts.empty())
		throw (ConfigBadConstrException("Config file is empty"));
	check_boundaries(cntnts);
	cntnts = extract_server_info(cntnts);
	if (cntnts.empty())
		throw (ConfigBadConstrException("Config file wrong format"));

	parse_required(cntnts);
	parse_optional(cntnts);
	this->_is_real = true;
	server_count(true, false);
	// std::cout << "CONTENT [" << cntnts << "]\n";
}

Config::ConfigBadConstrException::ConfigBadConstrException(const std::string &msg): _msg(msg) {}

const char	*Config::ConfigBadConstrException::what() const throw()
{
	return (this->_msg.c_str());
}

Config::ConfigBadConstrException::~ConfigBadConstrException() _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW{}

/*
	SHELL COMMANDS FOR TESTING WITH THIS MAIN:
	alias cpp_comp="c++ -Wall -Wextra -Werror -std=c++98 -g3 -I includes -I includes/config";
	cpp_comp src/config/Config.cpp && ./a.out nginx_test.conf nginx_test.conf nginx_test.conf
*/
int main(int argc, char *argv[])
{
	for (int index = 1; index < argc; index++)
	{
		std::cout << std::endl << "-----------------------------------\n" << std::endl;
		try
		{
			std::cout << "FILE [" << argv[index] << "]:\n";
			while (true)
			{
				Config	configFile(argv[index]);
				std::cout << "\nCORRECT READING OF SERVER Nº[" << server_count(false, false) << "] FROM FILE [" << argv[index] << "]:\n\n-----------------------------------\n\n";
				if (configFile.get_is_real() == false)
					break ;
			}
		}
		catch(const std::exception& e)
		{
			std::string		msg = e.what();
			std::cerr << "\t" << msg;
			if (msg == "No more servers to read")
				std::cerr << " from file [" << argv[index] << "]\n\tSuccessfully read [" << server_count(false, true) << "] server(s)";
			std::cerr << ".\n";
		}
	}
	std::cout << std::endl << "-----------------------------------\n" << std::endl;
	return (0);
}

Config::~Config()
{}

void	Config::add_location(std::string path, std::string content)
{
	for (size_t i = 0; i < this->_locations.size(); i++)
		if (this->_locations[i].get_path() == path)
			throw (ConfigBadConstrException("Config file contains duplicate Location entries"));
	
	Location	new_location(path, content);
	// if (this->validate_location())
	this->_locations.push_back(new_location);
}

bool	&Config::get_is_real()
{
	return (this->_is_real);
}

void	Config::set_is_real(bool new_data)
{
	this->_is_real = new_data;
}

int	&Config::get_port()
{
	return (this->_port);
}

void	Config::set_port(int new_data)
{
	this->_port = new_data;
}

int	*Config::get_host()
{
	return (this->_host);
}

void	Config::set_host(int *new_data)
{
	this->_host[0] = new_data[0];
	this->_host[1] = new_data[1];
	this->_host[2] = new_data[2];
	this->_host[3] = new_data[3];
}

std::string	&Config::get_root()
{
	return (this->_www_root);
}

void	Config::set_root(std::string new_data)
{
	this->_www_root = new_data;
}

std::string	&Config::get_index()
{
	return (this->_index);
}

void	Config::set_index(std::string new_data)
{
	this->_index = new_data;
}

std::string	&Config::get_server_name()
{
	return (this->_server_name);
}

void	Config::set_server_name(std::string new_data)
{
	this->_server_name = new_data;
}

size_t	&Config::get_client_max_body_size()
{
	return (this->_client_max_body_size);
}

void	Config::set_client_max_body_size(size_t new_data)
{
	this->_client_max_body_size = new_data;
}

bool	&Config::get_autoindex()
{
	return (this->_autoindex);
}

void	Config::set_autoindex(bool new_data)
{
	this->_autoindex = new_data;
}

t_err_page	&Config::get_error_pages()
{
	return (this->_error_pages);
}

void	Config::set_error_pages(t_err_page new_data)
{
	this->_error_pages = new_data;
}

std::vector<Location>	&Config::get_locations()
{
	return (this->_locations);
}

void	Config::set_locations(std::vector<Location> new_data)
{
	this->_locations = new_data;
}


std::string	&Config::get_server_address()
{
	return (this->_server_address);
}

void	Config::set_server_address(std::string new_data)
{
	this->_server_address = new_data;
}

int	&Config::get_listen_fd()
{
	return (this->_listen_fd);
}

void	Config::set_listen_fd(int new_data)
{
	this->_listen_fd = new_data;
}

