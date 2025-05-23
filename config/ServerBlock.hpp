#ifndef SERVERBLOCK_HPP
#define SERVERBLOCK_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <set>
#include <limits.h>
#include <iostream>
#include "ParsingUtils.hpp"
#include "LocationBlock.hpp"

// #define LOG_ERR(msg) std::cerr << "Erreur config\n" << msg << std::endl

// struct Listen
// {
//     std::string fullListenField; // 127.0.1.3:80
//     std::string host; // 127.0.1.3
//     unsigned int    port; // 80
// };

class ServerBlock 
{
    public:
        ServerBlock(): _host("0.0.0.0"), _port(80), _clientMaxBodySize(1000000) {};
        std::vector<Directive>      directives;
        std::vector<LocationBlock>  _locations;

        // getters
        std::string                 getHost() const{ return _host; };
        int getPort() const { return _port; };
        const std::string& getServerName() const { return _serverName; };
        // const std::set<std::string>& getServerNames() const { return _serverNames; }
        const std::string& getRoot() const { return _root; };
        const std::vector<std::string>& getIndex() const { return _index; };
        size_t getClientMaxBodySize() const { return _clientMaxBodySize;};
        const std::map<int, std::string>& getErrorPages() const { return _errorPages; };
        const std::vector<LocationBlock>& getLocations() const { return _locations; }
        const std::set<std::pair<std::string, int> >& getListen() const { return _listen; }

        bool    checkServerBlock();
        bool    checkLocationBlock(const LocationBlock& location);

        bool    validateListen(const Directive& directive);
        bool    isValidHost(const Directive& directive);
        bool    isValidClientBodySize(const Directive& directive);

        // Setters
        void setHost(const std::string& host) { _host = host; }
        void setPort(int port) { _port = port; }
        void setServerName(const std::string& name) { _serverName = name; }
        void setRoot(const std::string& root) { _root = root; }
        void setIndex(const std::string& index) { _index.clear(); _index.push_back(index); }
        void setIndexes(const std::vector<std::string>& indexes) { _index = indexes; } 
        void setClientMaxBodySize(size_t size) { _clientMaxBodySize = size; }
        void addErrorPage(int code, const std::string& path) { _errorPages[code] = path; }
        void addLocation(const LocationBlock& loc) { _locations.push_back(loc); }


        // trouver le bloc Location selon le chemin demander (a utiliser dans class Response)
        const LocationBlock* findMatchingLocation(const std::string& requestPath) const {
            for (size_t i = 0; i < _locations.size(); ++i) {
                if (requestPath.find(_locations[i].path) == 0)
                    return &_locations[i];
            }
            return NULL;
        }

        // Listen&                     listen;

    private:
        std::string                 _host;
        int                         _port;
        std::string                 _root;
        std::string                 _serverName;
        std::vector<std::string>                 _index;
        std::map<int, std::string>  _errorPages;
        size_t                 _clientMaxBodySize;
        std::set<std::pair<std::string, int> > _listen;
};

#endif