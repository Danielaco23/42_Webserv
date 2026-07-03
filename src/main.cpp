
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
int main(int argc, char **argv)
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);

    std::vector<Config> configs;

    try
    {
        if (argc == 2)
        {
            while (true)
            {
                Config cfg(argv[1]);

                if (!cfg.get_is_real())
                    break;

                configs.push_back(cfg);
            }
        }
        else
        {
            Config s1;

            int host1[4] = {127, 0, 0, 1};

            s1.set_host(host1);
            s1.set_port(8080);
            s1.set_root("www");
            s1.set_index("index.html");

            configs.push_back(s1);
        }
    }
    catch (const std::exception &e)
    {
        std::string msg = e.what();

        if (msg != "No more servers to read")
        {
            std::cerr << e.what() << std::endl;
            return (1);
        }
    }

    if (configs.empty())
    {
        std::cerr << "No valid servers loaded." << std::endl;
        return (1);
    }

    Server server(configs);

    server.initSocket();
    server.initVariables();
    server.startListening();

    std::cout << "Server started..." << std::endl;

    server.run();

    return (0);
}