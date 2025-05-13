#ifndef CONFIGFILE_HPP
 #define CONFIGFILE_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include "ParsingUtils.hpp"
#include "ServerBlock.hpp"
#include "Directive.hpp"


class ConfigFile {
    public:
        ConfigFile(){};

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



        bool    checkServers();

        
        void    displayConfig() const;
    
    // private:
        std::vector<ServerBlock> _servers;
        std::string _file;
};

#endif