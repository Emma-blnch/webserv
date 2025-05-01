#ifndef SERVERCONFIG_HPP
 #define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>

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

        bool    parseFile(const std::string& filename);
        void    displayConfig() const;
        
    private:
        std::vector<ServerBlock> _servers;
        std::string removeSpaces(const std::string& line);
        bool    isDirective(const std::string& line);
        bool    isBlockStart(const std::string& line);
        bool    isBlockEnd(const std::string& line);
        std::pair<std::string, std::string> parseDirective(const std::string& line);
};

#endif