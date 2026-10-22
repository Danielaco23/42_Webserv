#include "../includes/Server.hpp"

/**
 * @brief Main entry point of the Webserv application.
 * @param argc Number of command-line arguments.
 * @param argv Command-line arguments.
 * @return 0 on success, 1 on error.
 */
int main(int argc, char **argv)
{
    std::vector<Config> configs;

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);

    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv [configuration file]"
                  << std::endl;
        return (1);
    }

    try
    {
        while (true)
        {
            Config cfg(argv[1]);

            if (!cfg.get_is_real())
                break;

            configs.push_back(cfg);
        }
    }
    catch (const std::exception &e)
    {
        if (std::string(e.what()) != "No more servers to read")
        {
            std::cerr << "Configuration error: "
                      << e.what() << std::endl;
            return (1);
        }
    }

    if (configs.empty())
    {
        std::cerr << "Error: no valid server configuration found."
                  << std::endl;
        return (1);
    }

    try
    {
        Server server(configs);

        server.initSocket();
        server.initVariables();
        server.startListening();

        std::cout << "Webserv started." << std::endl;

        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: "
                  << e.what() << std::endl;
        return (1);
    }

    return (0);
}