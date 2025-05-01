#include "ServerConfig.hpp"
#include <fstream>
#include <iostream>


bool    ServerConfig::isBlockStart(const std::string& line){
    return line.find("{") != std::string::npos;
}

bool    ServerConfig::isDirective(const std::string& line){
    return line.find(";") != std::string::npos;
}

bool    ServerConfig::isBlockEnd(const std::string& line){
    return line.find("}") != std::string::npos;
}


std::string ServerConfig::removeSpaces(const std::string& line){
    if (line.empty()) // bool true if len = 0
        return line;
    
    size_t first = 0;
    while (first < line.length() && std::isspace(line[first]))
        first++;
    if (first == line.length())
        return ("");

    size_t last = line.length() - 1;
    while (std::isspace(line[last]))
        last--;
    return line.substr(first, last - first + 1);
}

std::pair<std::string, std::string>    ServerConfig::parseDirective(const std::string& line){
    std::pair<std::string, std::string> pair;

    std::string cleanLine = line;

    size_t  semiColonPos = cleanLine.find(';');
    if (semiColonPos != std::string::npos){
        cleanLine = cleanLine.substr(0, semiColonPos);
    }
    if (cleanLine.empty()){
        std::cout << "Empty directive\n";
    }
    size_t  spacePos = cleanLine.find_first_of(" \t");
    if (spacePos != std::string::npos){
        pair.first = cleanLine;
        pair.second = removeSpaces(cleanLine.substr(spacePos));
    } else {
        pair.first = cleanLine;
        pair.second = "";
    }
    return pair;
}

bool    ServerConfig::parseFile(const std::string& filename){
    std::ifstream   file(filename.c_str());

    if (!file.is_open())
    {
        std::cout << "Cannot open the file\n";
        return false;
    }

    std::string line;
    bool    inServerBlock = false;
    bool    inLocationBlock = false;
    ServerBlock currentServer;
    LocationBlock   currentLocation;

    while (std::getline(file, line)){ // std::getline(input, string, delimeter): par défaut délim est \n et n'est pas inclus
        line = removeSpaces(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (!inServerBlock && line.find("server") != std::string::npos && isBlockStart(line)){
            inServerBlock = true;
            currentServer = ServerBlock(); // reset currentServer
            continue ;
        }
        if (inServerBlock && !inLocationBlock && isBlockEnd(line))
        {
            inServerBlock = false;
            _servers.push_back(currentServer); //on l'ajoute au vecteur de ServerBlocks
            continue;
        }
        if (inServerBlock){
            if (!inLocationBlock && line.find("location") != std::string::npos && isBlockStart(line)){
                inLocationBlock = true;
                currentLocation = LocationBlock();

                size_t  startPos = line.find("location") + 8;
                size_t  endPos = line.find('{');
                if (endPos != std::string::npos){
                    currentLocation.path = removeSpaces(line.substr(startPos, endPos - startPos));
                }
                continue;
            }
            if (inLocationBlock && isBlockEnd(line)){
                inLocationBlock = false;
                currentServer.locations.push_back(currentLocation);
                continue ;
            }
        }

        if (isDirective(line)){
            std::pair<std::string, std::string> directive = parseDirective(line);
            if (inLocationBlock) {
                Directive dir;
                dir.key = directive.first;
                dir.value = directive.second;
                currentLocation.directives.push_back(dir);
                continue;
            }
            else {
                Directive dir;
                dir.key = directive.first;
                dir.value = directive.second;
                currentServer.directives.push_back(dir);
                continue;
            }
        }
    }
    file.close();
    return true;
}
