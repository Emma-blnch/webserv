#include "ServerConfig.hpp"

bool    ServerConfig::isValidClientBodySize(const Directive& directive)
{
    if (directive.key.empty())
    {
        std::cout << "Erreur config: manque client max body size ??\n";
        return false;
    }
}

bool    ServerConfig::isValidPort(const Directive& directive)
{
    if (directive.value.empty()) // port =  directive.value
    {
        std::cout << "Erreur config: listen to what ??\n";
        return false;
    }
    size_t  i = 0;
    while(directive.value[i])
    {
        if (!std::isdigit(directive.value[i]))
            return false;
        i++;
    }
    long    result = std::stol(directive.value);
    if (result < INT_MIN || result > INT_MAX || result <= 0 || result > 65535)
    {
        std::cout << "Erreur config: le port " << directive.value << " n'est pas valide\n";
        return false;
    }
    return true;
}

// TO DO doublons à vérifier (check le booléen)
bool    ServerConfig::checkServerBlock(const ServerBlock& server)
{
    bool    hasListen = false; // format "listen host:port", listen port = listen 0.0.0.0:port
    bool    hasClientMaxSize = false;
    bool    hasErrorPages = false;
    bool    hasServerName = false; // pas obligatoire
    // TO DO

    for (size_t i = 0; i < server.directives.size(); ++i)
    {
        const Directive& currentDir = server.directives[i];
        if (currentDir.key == "listen")
        {
            if (!isValidPort(currentDir))
                return false;
            hasListen = true;
        }
        else if (currentDir.key == "client_max_body_size") 
        {
            if (!isValidClientBodySize(currentDir))
                return false;
            hasClientMaxSize = true;
        }
        else if (currentDir.key == "server_name")
        {
            if (currentDir.value.empty())
                std::cout << "Attention pas de server_name mais pas obligatoire\n";
        }
        else if (currentDir.key == "error_page")
        {
            if (currentDir.value.empty())
            {
                std::cout << "Erreur config: manque page(s) d'erreur\n";
                return false;
            }
        }
        else
        {
            std::cout << "Directive non autorisée: " << server.directives[i].key << std::endl;
            return false;
        }
    }

    for (size_t i = 0; i < server.locations.size(); ++i){
        if (!checkLocationBlock(server.locations[i]))
            return false;
    }
    return true;
}

bool    ServerConfig::checkServers()
{
    for (size_t i = 0; i < _servers.size(); ++i){
        if (!checkServerBlock(_servers[i]))
            return false;
    }
    return true;
}

bool    ServerConfig::checkServers()
{
    for (size_t i = 0; i < _servers.size(); ++i){
        if (!checkServerBlock(_servers[i]))
            return false;
    }
    return true;
}