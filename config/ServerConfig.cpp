#include "ServerConfig.hpp"

bool    ServerConfig::isServerBlockStart(std::vector<std::string> tokens)
{
    return ((tokens.size() == 2 && tokens[0] == "server" && tokens[1] == "{")
    || (tokens.size() == 1 && tokens[0] == "server{"));
}

bool    ServerConfig::isLocationBlockStart(std::vector<std::string> tokens)
{
    if (tokens.size() == 3 && tokens[0] == "location" && tokens[2] == "{"){
        return true;
    }
    return false;
}

bool    ServerConfig::isBlockEnd(const std::string& line)
{
    std::string trimmedLine = removeCommentsAndEndSpaces(line);
    return (trimmedLine == "}");
}


bool    ServerConfig::isDirective(const std::string& line)
{
    return line.find(';') != std::string::npos;
}

std::pair<std::string, std::string> ServerConfig::parseDirective(const std::string& line)
{
    std::pair<std::string, std::string> result;

    std::string cleanLine = line;
    size_t  semiColonPos = cleanLine.find(';');
    if (semiColonPos != std::string::npos)
        cleanLine = cleanLine.substr(0, semiColonPos);
    std::vector<std::string> tokens = splitLine(cleanLine, " \t");
    if (tokens.empty())
        return result;
    result.first = tokens[0];
    if (tokens.size() > 1){
        result.second = tokens[1];
        for (size_t i = 2; i < tokens.size(); ++i){
            result.second += " " + tokens[i];
        }
    }
    return result;
}

bool    ServerConfig::extractDirective(std::string& line, Directive& dir)
{
    std::pair<std::string, std::string> directive = parseDirective(line);
    if (directive.first.empty())
    {
        std::cout << "Erreur config: directive vide\n";
        return false;
    }
    dir.key = directive.first;
    dir.value = directive.second;
    return true;
}


bool    ServerConfig::extractLocationBlockContent(LocationBlock& location, std::ifstream& file)
{
    std::string line;

    while (std::getline(file, line))
    {
        line = removeCommentsAndEndSpaces(line);
        if (line.empty() || line[0] == '#')
        if (isBlockEnd(line))
            return true;
        std::vector<std::string>    tokens = splitLine(line, " \t");
        if (isServerBlockStart(tokens)|| isLocationBlockStart(tokens))
        {
            std::cout << "Erreur config: bloc trouvé dans un bloc location\n";
            return false;
        }
        if (isDirective(line)){
            Directive directive;
            if (!extractDirective(line, directive))
                return false;
            location.directives.push_back(directive);
        }
        else
        {
            std::cout << "Erreur config: Un bloc location ne doit contenir que des directives\n";
            return false;
        }
    }
    std::cout << "Erreur config: bloc location ouvert\n";
    return false;
}


bool    ServerConfig::extractServerBlockContent(ServerBlock& server, std::ifstream& file)
{
    std::string line;
    
    while (std::getline(file, line))
    {
        line = removeCommentsAndEndSpaces(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (isBlockEnd(line))
            return true;
        std::vector<std::string>    tokens = splitLine(line, " \t");
        if (isServerBlockStart(tokens)){
            std::cout << "Erreur config: un bloc server ne peut en contenir un autre\n";
            return false;
        }
        if (isLocationBlockStart(tokens)){
            LocationBlock   location;
            std::string path = tokens[1];
            location.path = path;
            if (!extractLocationBlockContent(location, file))
                return false;
            server.locations.push_back(location);
        }
        else if (isDirective(line))
        {
            Directive   directive;
            if (!extractDirective(line, directive))
                return false;
            server.directives.push_back(directive);
            std::cout << "Server directive: " << directive.key << " " << directive.value << std::endl;
        }
        else
        {
            std::cout << "Un bloc serveur ne doit contenir que des directives et des blocs location\n";
            return false;
        }
    }
    std::cout << "Bloc server ouvert\n";
    return false;
}


bool    ServerConfig::extractServerBlocks(std::ifstream& file)
{
    std::string line;
    ServerBlock currentServer;

    while (std::getline(file, line)){
        line = removeCommentsAndEndSpaces(line);
        if (line.empty() || line[0] == '#')
            continue ;
        std::vector<std::string>    tokens = splitLine(line, " \t");
        if (!isServerBlockStart(tokens)){
            std::cout << "Erreur config: n'écrire que dans les blocs server\n";
            return false;
        }
        else
        {
            currentServer = ServerBlock();
            if (!extractServerBlockContent(currentServer, file))
                return false;
            _servers.push_back(currentServer);
            std::cout << "Added server block\n";
        }
    }
    if (_servers.empty())
    {
        std::cout << "Erreur config: aucun serveur trouvé\n";
        return false;
    }
    return true;
}


bool    ServerConfig::checkLocationBlock(const LocationBlock& location)
{
    (void)location;
    return true;
}

bool    ServerConfig::checkServerBlock(const ServerBlock& server)
{
    // bool    hasListen = false;
    // bool    hasHost = false;
    // bool    hasClientMaxSize = false;
    // bool    hasErrorPages = false;
    // bool    hasServerName = false; // pas obligatoire
    // TO DO



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

bool    ServerConfig::parseConfigFile(const std::string& filename)
{
    std::ifstream file(filename.c_str());

    if (!file.is_open()){
        std::cout << "Could not open "<< filename << "\n";
        return false;
    }

    if (!extractServerBlocks(file)){
        file.close();
        return false;
    }
    file.close();
    return checkServers();
}