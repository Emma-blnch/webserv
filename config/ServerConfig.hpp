#ifndef SERVERCONFIG_HPP
 #define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include "ParsingUtils.hpp"


struct Directive{
    std::string key;
    std::string value;
    
};

struct LocationBlock{
    std::string path;
    std::vector<Directive> directives;
};

struct ServerBlock{
    std::vector<Directive> directives;
    std::vector<LocationBlock> locations;
};


class ServerConfig {
    public:
        ServerConfig(){};

        bool    parseConfigFile(const std::string& filename);
        bool    extractServerBlocks(std::ifstream& file);
        bool    extractServerBlockContent(ServerBlock& server, std::ifstream& file);

        std::pair<std::string, std::string> parseDirective(const std::string& line);
        bool    extractDirective(std::string& line, Directive& dir);


        bool    isLocationBlockStart(std::vector<std::string> tokens);
        bool    extractLocationBlockContent(LocationBlock& location, std::ifstream& file);

        bool    isDirective(const std::string& line);
        bool    isServerBlockStart(std::vector<std::string> tokens);
        bool    isBlockEnd(const std::string& line);

        bool    checkServerBlock(const ServerBlock& server);
        bool    checkLocationBlock(const LocationBlock& location);
        bool    checkServers();

        
        void    displayConfig() const;
        
    private:
        std::vector<ServerBlock> _servers;
};

#endif