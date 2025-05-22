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
    if (tokens.size() == 3 && tokens[0] == "location" && tokens[2] == "{"){
        return true;
    }
    return false;
}

bool    ConfigFile::isBlockEnd(const std::string& line)
{
    std::string trimmedLine = removeCommentsAndEndSpaces(line);
    return (trimmedLine == "}");
}


bool    ConfigFile::isDirective(const std::string& line)
{
    return line.find(';') != std::string::npos;
}

std::pair<std::string, std::string> ConfigFile::parseDirective(const std::string& line)
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
        LOG_ERR("Directive vide");
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
        std::cout << "location block line: " << line << std::endl; 
        if (line.empty() || line[0] == '#')
            continue;
        std::vector<std::string>    tokens = splitLine(line, " \t");
        if (isServerBlockStart(tokens)|| isLocationBlockStart(tokens))
        {
            LOG_ERR("Bloc trouvé dans un bloc location");
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
            std::cout << "block end\n";
            return true;
        }
        else
        {
            LOG_ERR("Un bloc location ne doit contenir que des directives");
            return false;
        }
    }
    LOG_ERR("Bloc location ouvert");
    return false;
}


bool    ConfigFile::extractServerBlockContent(ServerBlock& server, std::ifstream& file)
{
    std::string line;
    
    while (std::getline(file, line))
    {
        line = removeCommentsAndEndSpaces(line);
        std::cout << line << std::endl;
        if (line.empty() || line[0] == '#')
            continue;
        if (isBlockEnd(line))
            return true;
        std::vector<std::string>    tokens = splitLine(line, " \t");
        if (isServerBlockStart(tokens)){
            LOG_ERR("Un bloc server ne peut en contenir un autre");
            return false;
        }
        if (isLocationBlockStart(tokens)){
            LocationBlock   location;
            std::string path = tokens[1];
            location.path = path;
            // int fd = open(path.c_str(), O_DIRECTORY);
            // if (fd < 0) {
            //     std::cout << "Erreur: ce n'est pas un dossier\n";
            //     return false;
            // }
            // close(fd);
            // -> peut etre un path logique HTTP pas un repertoire réel ex:
            // location /foo/ {
            //       root www/dir;
            //  }
            // = l'url sera www/dir/fichier
            if (!extractLocationBlockContent(location, file))
                return false;
            // server._locations.push_back(location);
            server.addLocation(location);
        }
        else if (isDirective(line))
        {
            Directive   directive;
            if (!extractDirective(line, directive))
                return false;
            server.directives.push_back(directive);
            // std::cout << "Server directive: " << directive.key << " " << directive.value << std::endl;
        }
        else
        {
            LOG_ERR("Un bloc serveur ne doit contenir que des directives et des blocs location");
            return false;
        }
    }
    std::cout << "Bloc server ouvert\n";
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
            LOG_ERR("N'écrire que dans les blocs server");
            return false;
        }
        else
        {
            currentServer = ServerBlock();
            if (!extractServerBlockContent(currentServer, file))
                return false;
            _servers.push_back(currentServer);
            // std::cout << "Added server block\n";
        }
    }
    if (_servers.empty())
    {
        LOG_ERR("Aucun serveur trouvé");
        return false;
    }
    return true;
}

bool    ConfigFile::checkServers()
{
    for (size_t i = 0; i < _servers.size(); ++i){
        // ServerBlock s; // sert juste à appeler la méthode
        // if (!s.checkServerBlock(_servers[i]))
        if (!_servers[i].checkServerBlock(_servers[i]))
            return false;
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
    _file = filename;

    if (!extractServerBlocks(file)){
        file.close();
        return false;
    }
    file.close();
    return checkServers(); // checkServerBlock.cpp
}