#include "ServerManager.hpp"

ServerManager::ServerManager(const Config &cfg)
{
    // 🔥 AQUÍ SE CONSTRUYEN LOS SERVERS DESDE CONFIG
    // (ahora mismo simplificado)

    Server s(cfg);
    servers.push_back(s);
}

void ServerManager::run()
{
    while (true)
    {
        for (size_t i = 0; i < servers.size(); i++)
        {
            servers[i].runOnce(); // 👈 IMPORTANTE: NO while(true)
        }
    }
}