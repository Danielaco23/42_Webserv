
#include "Webserver.hpp"

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
	Config config;

	if (argc != 2)
		return (print_cerr<std::string>(ARG_ERR), 1);
	print_cout<std::string>(argv[1]);

	// TODO: Aquí se debería crear el objeto Config, parsear el archivo de configuración y luego iniciar el servidor con esa configuración.
	// config.parseConfigFile(argv[1]);

	return (0);
}
