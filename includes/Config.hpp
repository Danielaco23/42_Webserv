
#pragma once

#include <string> // HTTP petition handler
#include <vector> // Client list
#include <map> // HTTP headers

class LocationConfig
{
    public:
        std::string path;
        std::string root;
        std::vector<std::string> index;
        bool autoindex;
        std::vector<std::string> allowed_methods;
        
        std::string cgi_extension;
        std::string cgi_path;
        std::string upload_path;
        
        LocationConfig();
};

class ServerConfig
{
    public:
        std::string host;
        int port; //=listen

        std::vector<std::string> server_name;
        std::string root;
        std::vector<std::string> index;
        size_t client_max_body_size;
        std::map<int, std::string> error_pages;
        
        std::vector<LocationConfig> locations;
        
        ServerConfig();
};

class Config
{
    public:
        std::vector<ServerConfig> servers;
    
        Config();
};
