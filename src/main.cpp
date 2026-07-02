
// #include "Webserver.hpp"
#include "../includes/Server.hpp"



template <typename T>
/**
 * @brief This function prints a given string to the standard output
 * @param printable String to be printed to the std::cout
 */
void	print_cout(T printable)
{
	std::cout << printable << std::endl;
}

template <typename T>
/**
 * @brief This function prints a given string, in red, to the error output
 * @param printable String to be printed to the std::cerr
 */
void	print_cerr(T printable)
{
	const std::string color_R = "\033[0;31m";
	const std::string color_NC = "\033[0m";
	std::cerr << color_R << printable << color_NC << std::endl;
}

/**
 * @brief The main function of the project, kickstarts the execution and begins the program.
 * @param argc Number of arguments, including the executable name.
 * @param argv Constant array of C strings,
 * containing the arguments in order of introduction, with position 0 being the executable mane.
 */
int main(int argc, char const *argv[])
{
	// Config config;
	
	(void)argv;
	(void)argc;
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGQUIT, signalHandler);
	// if (argc != 2)
	// 	return (print_cerr<std::string>(ARG_ERR), 1);
	// print_cout<std::string>(argv[1]);

	std::vector<ServerConfig> configs;

	ServerConfig s1;
	s1.host = "127.0.0.1";
	s1.port = 8080;
	s1.root = "www";
	s1.index = "index.html";

	ServerConfig s2;
	s2.host = "127.0.0.1";
	s2.port = 8081;
	s2.root = "www";
	s2.index = "index.html";

	configs.push_back(s1);
	configs.push_back(s2);

	Server server(configs);

    server.initSocket();
	server.initVariables();
	server.startListening();

	std::cout << "Server started..." << std::endl;

    server.run();

    return (0);

	// TODO: Aquí se debería crear el objeto Config, parsear el archivo de configuración y luego iniciar el servidor con esa configuración.
	// config.parseConfigFile(argv[1]);
}
