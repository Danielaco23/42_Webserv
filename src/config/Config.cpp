
#include "Config.hpp"

#define FILE_READ_BUFFER 256

static void	buff_reset(char *str)
{
	for (size_t i = 0; i < FILE_READ_BUFFER; i++)
		str[i] = '\0';
}

static std::string	read_whole_file(std::string file)
{
	std::ifstream	input;
	char			buffer[FILE_READ_BUFFER];
	std::string		content;

	input.open(file.c_str());

	if (!input.is_open())
		return (std::cerr << "Failed to open file" << std::endl, "");
	buffer[0] = 'a';
	while (buffer[0])
	{
		buff_reset(buffer);
		input.read(buffer, (FILE_READ_BUFFER - 1));
		content = content + buffer;
	}
	input.close();
	return (content);
}

static std::string	extract_from_brackets(std::string content)
{
	size_t			start = content.find('{');
	size_t			possible_end = content.find('}');
	size_t			cp = start;

	if (possible_end == content.npos || start == content.npos)
		return ("");
	while (possible_end != content.npos && cp < possible_end)
	{
		cp = content.find('{', possible_end + 1);
		possible_end = content.find('}', possible_end + 1);
	}
	return (content.substr(start + 1, (possible_end - 1) - (start + 1)));
}

void	rmv_char_onwards(std::string &contents, std::string dth_mrk)
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

Config::Config(std::string config_file)
{
	(void) this->_port;
	(void) this->_client_max_body_size;
	(void) this->_autoindex;
	(void) this->_listen_fd;

	if (config_file.empty() || config_file.size() < 6)
		throw (std::cerr << "Config file name too small" << std::endl, ConfigBadConstructException());
	if (config_file.substr(config_file.size() - 5) != ".conf")
		throw (std::cerr << "Config file wrong file extension." << std::endl, ConfigBadConstructException());

	std::string		file_contents = read_whole_file(config_file);
	if (file_contents.empty())
		throw (ConfigBadConstructException());
	rmv_char_onwards(file_contents, "#");

	size_t	loc = file_contents.find("server");
	if (!(loc != file_contents.npos && std::isspace(file_contents[loc + 6])))
		throw (std::cerr << "Element server not present." << std::endl, ConfigBadConstructException());

	file_contents = extract_from_brackets(file_contents);
	
	std::cout << file_contents << std::endl;
	/*size_t pos = file_contents.find("server ", 0);
	if (pos == file_contents.npos)
		throw (std::cerr << "File does not contain server." << std::endl, ConfigBadConstructException());
	std::cout << file_contents << std::endl;*/
	// this->_port = std::stoi(file_contents.substr(pos + 7, 4));
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
		std::cout << std::endl << "-----------------------------------" << std::endl << std::endl;
		try
		{
			std::cout << "FILE [" << argv[index] << "]:" << std::endl;
			Config	configFile(argv[index]);
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
	return (0);
}

Config::~Config()
{}
