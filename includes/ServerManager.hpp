#pragma once

#include <vector>
#include "Server.hpp"
#include "Config.hpp"

class ServerManager
{
    private:
        std::vector<Server> servers;

    public:
        ServerManager(const Config &cfg);
        void run();
};