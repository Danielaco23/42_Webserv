#include "Config_jh.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>

#define BUFFER 256

// ===============================
// READ FILE
// ===============================
std::string Config::readFile(const std::string &file)
{
    std::ifstream input(file.c_str());
    std::stringstream ss;

    if (!input.is_open())
        return "";

    ss << input.rdbuf();
    return ss.str();
}

// ===============================
// REMOVE COMMENTS (#)
// ===============================
void Config::removeComments(std::string &content)
{
    for (size_t i = 0; i < content.size(); i++)
    {
        if (content[i] == '#')
        {
            while (i < content.size() && content[i] != '\n')
                content.erase(i, 1);
        }
    }
}

// ===============================
// SPLIT SERVER BLOCKS
// ===============================
std::vector<std::string> Config::splitServers(const std::string &content)
{
    std::vector<std::string> servers;

    size_t pos = 0;

    while (true)
    {
        pos = content.find("server", pos);
        if (pos == std::string::npos)
            break;

        size_t open = content.find('{', pos);
        size_t close = content.find('}', open);

        if (open == std::string::npos || close == std::string::npos)
            break;

        servers.push_back(content.substr(open + 1, close - open - 1));

        pos = close;
    }

    return servers;
}

// ===============================
// PARSE SINGLE SERVER BLOCK
// ===============================

static std::string extractValue(
    const std::string &block,
    const std::string &key)
{
    size_t pos = block.find(key);

    if (pos == std::string::npos)
        return "";

    pos += key.size();

    while (pos < block.size()
        && std::isspace(static_cast<unsigned char>(block[pos])))
        pos++;

    size_t end = block.find(';', pos);

    if (end == std::string::npos)
        return "";

    return block.substr(pos, end - pos);
}

static int toInt(const std::string &str)
{
    std::istringstream iss(str);
    int value = 0;
    iss >> value;
    return value;
}

ServerConfig Config::parseServerBlock(const std::string &block)
{
    ServerConfig sc;

    // ======================
    // Defaults
    // ======================

    sc.port = 8080;
    sc.host = "127.0.0.1";
    sc.root = "www";
    sc.index = "index.html";
    sc.server_name = "webserv";
    sc.client_max_body_size = 50 * 1024 * 1024;
    sc.autoindex = false;

    // ======================
    // listen
    // ======================

    std::string value;

    value = extractValue(block, "listen");
    if (!value.empty())
        sc.port = toInt(value);

    // ======================
    // root
    // ======================

    value = extractValue(block, "root");
    if (!value.empty())
        sc.root = value;

    // ======================
    // index
    // ======================

    value = extractValue(block, "index");
    if (!value.empty())
        sc.index = value;

    // ======================
    // server_name
    // ======================

    value = extractValue(block, "server_name");
    if (!value.empty())
        sc.server_name = value;

    // ======================
    // client_max_body_size
    // ======================

    value = extractValue(block, "client_max_body_size");
    if (!value.empty())
        sc.client_max_body_size =
            static_cast<size_t>(toInt(value));

    // ======================
    // autoindex
    // ======================

    value = extractValue(block, "autoindex");
    if (value == "on")
        sc.autoindex = true;
    else if (value == "off")
        sc.autoindex = false;

    return sc;
}

// ===============================
// CONSTRUCTOR PRINCIPAL
// ===============================
Config::Config(const std::string &config_file)
{
    if (config_file.empty() || config_file.size() < 6)
        throw ConfigBadConstructException();

    if (config_file.substr(config_file.size() - 5) != ".conf")
        throw ConfigBadConstructException();

    std::string content = readFile(config_file);
    if (content.empty())
        throw ConfigBadConstructException();

    // 1. limpiar comentarios
    removeComments(content);

    // 2. separar servers
    std::vector<std::string> blocks = splitServers(content);

    if (blocks.empty())
        throw ConfigBadConstructException();

    // 3. parsear cada server
    for (size_t i = 0; i < blocks.size(); i++)
    {
        ServerConfig sc = parseServerBlock(blocks[i]);
        _servers.push_back(sc);
    }

    std::cout << "Loaded servers: " << _servers.size() << std::endl;
}

// ===============================
// GETTER (IMPORTANTE PARA CORE)
// ===============================
const std::vector<ServerConfig> &Config::getServers() const
{
    return _servers;
}

// ===============================
Config::~Config()
{}

// ===============================
const char *Config::ConfigBadConstructException::what() const throw()
{
    return "Config parsing error";
}