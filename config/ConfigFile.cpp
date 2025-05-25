#include "ConfigFile.hpp"
#include "ServerBlock.hpp"
#include <fcntl.h>

bool    ConfigFile::isServerBlockStart(std::vector<std::string> tokens)
{
    return ((tokens.size() == 2 && tokens[0] == "server" && tokens[1] == "{")
    || (tokens.size() == 1 && tokens[0] == "server{"));
}

bool    ConfigFile::isLocationBlockStart(std::vector<std::string> tokens)
{
    return (tokens.size() == 3 && tokens[0] == "location" && tokens[2] == "{");
}

bool    ConfigFile::isBlockEnd(const std::string& line)
{
    return (line == "}");
}

bool    ConfigFile::isDirective(const std::string& line)
{
    return line.find(';') != std::string::npos;
}

std::pair<std::string, std::string> ConfigFile::parseDirective(const std::string& line)
{
    std::string cleanLine = line;
    size_t  semiColonPos = cleanLine.find(';');

    if (semiColonPos != std::string::npos)
        cleanLine = cleanLine.substr(0, semiColonPos);

    std::pair<std::string, std::string> result;
    std::vector<std::string> tokens = splitLine(cleanLine, " \t");
    if (tokens.empty())
        return result;
    result.first = tokens[0];
    for (size_t i = 0; i < result.first.size(); ++i){
        if (std::isalpha(result.first[i]) && std::isupper(result.first[i])){
            result.first[i] = std::tolower(result.first[i]);
        }
    }
    if (tokens.size() > 1){
        result.second = tokens[1];
        for (size_t i = 2; i < tokens.size(); ++i){
            result.second += " " + tokens[i];
        }
    }
    return result;
}

bool    ConfigFile::extractDirective(std::string& line, Directive& dir)
{
    std::pair<std::string, std::string> directive = parseDirective(line);
    if (directive.first.empty() || directive.second.empty())
    {
        LOG_ERR("Empty directive");
        return false;
    }
    dir.key = directive.first;
    dir.value = directive.second;
    return true;
}


bool    ConfigFile::extractLocationBlockContent(LocationBlock& location, std::ifstream& file)
{
    std::string line;

    while (std::getline(file, line))
    {
        line = removeCommentsAndEndSpaces(line);
        if (line.empty() || line[0] == '#')
            continue;
        std::vector<std::string>    tokens = splitLine(line, " \t");
        if (isServerBlockStart(tokens)|| isLocationBlockStart(tokens))
        {
            LOG_ERR("Found a block in location block");
            return false;
        }
        if (isDirective(line)){
            Directive directive;
            if (!extractDirective(line, directive))
                return false;
            location.directives.push_back(directive);
        }
        else if (isBlockEnd(line))
        {
            return true;
        }
        else
        {
            LOG_ERR("A location block can only have directives");
            return false;
        }
    }
    LOG_ERR("Found an open location block");
    return false;
}


bool    ConfigFile::extractServerBlockContent(ServerBlock& server, std::ifstream& file)
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
            LOG_ERR("A server block can have nothing but directives and location blocks");
            return false;
        }
        if (isLocationBlockStart(tokens)){
            LocationBlock   location;
            std::string path = tokens[1];
            location.path = path;
            if (!extractLocationBlockContent(location, file))
                return false;
            server.addLocation(location);
        }
        else if (isDirective(line))
        {
            Directive   directive;
            if (!extractDirective(line, directive))
                return false;
            server.directives.push_back(directive);
        }
        else
        {
            LOG_ERR("A server block can have nothing but directives and location blocks");
            return false;
        }
    }
    LOG_ERR("Found an open server block");
    return false;
}

bool    ConfigFile::extractServerBlocks(std::ifstream& file)
{
    std::string line;
    ServerBlock currentServer;

    while (std::getline(file, line)){
        line = removeCommentsAndEndSpaces(line);
        if (line.empty() || line[0] == '#')
            continue ;
        std::vector<std::string>    tokens = splitLine(line, " \t");
        if (!isServerBlockStart(tokens)){
            LOG_ERR("Cannot write outside server blocks");
            return false;
        }
        else
        {
            currentServer = ServerBlock();
            if (!extractServerBlockContent(currentServer, file))
                return false;
            _servers.push_back(currentServer);
        }
    }
    if (_servers.empty())
    {
        LOG_ERR("No server has been found");
        return false;
    }
    return true;
}

bool    ConfigFile::checkServers()
{
    for (size_t i = 0; i < _servers.size(); ++i)
    {
        if (!_servers[i].checkServerBlock())
            return false;
    }
    // ---- CHECK DES DOUBLONS DE SERVER_NAME ----
    std::set<std::string> allNames;
    for (size_t i = 0; i < _servers.size(); ++i) {
        const std::vector<std::string>& names = _servers[i].getServerNames();
        for (size_t j = 0; j < names.size(); ++j) {
            if (allNames.count(names[j])) {
                std::cerr << "Duplicate server_name '" << names[j] << "' found." << std::endl;
                return false;
            }
            allNames.insert(names[j]);
        }
    }
    return true;
}


bool    ConfigFile::parseConfigFile(const std::string& filename)
{
    std::ifstream file(filename.c_str());

    if (!file.is_open()){
        std::cerr << "Could not open "<< filename << "\n";
        return false;
    }
    if (!extractServerBlocks(file)){
        file.close();
        return false;
    }
    file.close();
    return checkServers();
}