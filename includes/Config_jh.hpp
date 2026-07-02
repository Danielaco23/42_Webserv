#pragma once

#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <string>
# include <map>
# include <vector>
# include <exception>

/*
    Esta estructura es CLAVE:
    - representa UN server block del config
    - el core la usará directamente
*/
struct ServerConfig
{
    unsigned int port;
    std::string host;
    std::string root;
    std::string index;
    std::string server_name;

    unsigned long client_max_body_size;
    bool autoindex;

    std::map<int, std::string> error_pages;
};

class Config
{
    private:
        std::vector<ServerConfig> _servers;

        // helpers parsing
        std::string readFile(const std::string &file);
        void removeComments(std::string &content);
        std::vector<std::string> splitServers(const std::string &content);
        ServerConfig parseServerBlock(const std::string &block);

    public:
        Config();
        Config(const std::string &config_file);
        ~Config();

        const std::vector<ServerConfig> &getServers() const;

        class ConfigBadConstructException : public std::exception
        {
            public:
                virtual const char *what() const throw();
        };
};

#endif