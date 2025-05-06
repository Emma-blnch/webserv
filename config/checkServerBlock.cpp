#include "ServerConfig.hpp"

bool    ServerConfig::isValidClientBodySize(const Directive& directive)
{
    if (directive.value.empty())
    {
        std::cout << "Erreur config: manque client max body size ??\n";
        return false;
    }
    std::string size = directive.value;
    size_t  i = 0;
    while (i < size.length() && std::isdigit(size[i]))
        i++;
    if (i == 0) // aucun chiffre
        return false;
    if (i == size.length()) // que des chiffres, juste à vérifier l'overflow
    {
        long    result = std::stol(size);
        if (result < INT_MIN ||  result > INT_MAX)
        {
            std::cout << "Erreur config: bodymaxsize < intmin ou > intmax\n";
            return false;
        }
        return true;
    }
    std::string unit = size.substr(i); // prends juste les chiffres de la string (car par ex 1M ou 1G est valide)
    return (unit == "k" || unit == "K" || unit == "m" || unit == "M" || 
        unit == "g" || unit == "G");
}

bool    ServerConfig::isValidPort(const Directive& directive)
{
    if (directive.value.empty()) // port =  directive.value
    {
        std::cout << "Erreur config: listen to what ??\n";
        return false;
    }
    size_t  i = 0;
    while(i < directive.value.length())
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
    // bool    hasListen = false; // format "listen host:port", listen port = listen 0.0.0.0:port
    // bool    hasClientMaxSize = false;
    // bool    hasErrorPages = false;
    // bool    hasServerName = false; // pas obligatoire
    // TO DO

    for (size_t i = 0; i < server.directives.size(); ++i)
    {
        const Directive& currentDir = server.directives[i];
        if (currentDir.key == "listen")
        {
            if (!isValidPort(currentDir))
                return false;
            // hasListen = true;
        }
        else if (currentDir.key == "client_max_body_size") 
        {
            if (!isValidClientBodySize(currentDir))
                return false;
            // hasClientMaxSize = true;
        }
        else if (currentDir.key == "server_name")
        {
            if (currentDir.value.empty())
                std::cout << "Attention pas de server_name mais pas obligatoire\n";
            // hasServerName = true;
        }
        else if (currentDir.key == "error_page")
        {
            if (currentDir.value.empty())
            {
                std::cout << "Erreur config: manque page(s) d'erreur\n";
                return false;
            }
            // hasErrorPages = true;
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