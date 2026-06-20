// IN CURRENT, ACTIVE DEVELOPMENT
#include "Config.hpp"

#define FILE_READ_BUFFER 256

static void	buff_reset(char *str)
{
	for (size_t i = 0; i < FILE_READ_BUFFER; i++)
		str[i] = '\0';
}
// IN CURRENT, ACTIVE DEVELOPMENT
static std::string	read_whole_file(std::string file)
{
	std::ifstream	input;
	char			buffer[FILE_READ_BUFFER];
	std::string		content;

	input.open(file.c_str());

	if (!input.is_open())
		return ("");
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
// IN CURRENT, ACTIVE DEVELOPMENT
Config::Config(std::string config_file)
{
	std::string		file_contents = read_whole_file(config_file);
	
	if (file_contents.empty())
	throw (ConfigBadConstructException());
	//std::cout << file_contents << std::endl;
	
	size_t pos = file_contents.find("listen ", 0);
}
// IN CURRENT, ACTIVE DEVELOPMENT
const char	*Config::ConfigBadConstructException::what() const throw()
{
	return ("Could not construct config class.");
}
// IN CURRENT, ACTIVE DEVELOPMENT
Config::~Config()
{}
// IN CURRENT, ACTIVE DEVELOPMENT
/*int main(void)
{
	try
	{
		std::cout << "Start" << std::endl;
		Config	configFile("nginx.conf");
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	std::cout << "End" << std::endl;
	return (0);
}*/
