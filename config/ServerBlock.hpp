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

class ServerBlock 
{
    public:
        ServerBlock(): _host("0.0.0.0"), _port(80), _clientMaxBodySize(1000000) {};
        std::vector<Directive>      directives;
        std::vector<LocationBlock>  _locations;

        bool    checkServerBlock();
        bool    checkLocationBlock(const LocationBlock& location);

        bool    validateListen(const Directive& directive);
        bool    isValidHost(const Directive& directive);
        bool    isValidClientBodySize(const Directive& directive);

        // Setters
        void    setHost(const std::string& host) { _host = host; }
        void    setPort(int port) { _port = port; }
        void    setRoot(const std::string& root) { _root = root; }
        void    setIndexes(const std::vector<std::string>& indexes) { _index = indexes; } 
        void    setServerNames(const std::vector<std::string>& names) { _serverName = names; }
        void    setClientMaxBodySize(size_t size) { _clientMaxBodySize = size; }
        void    addErrorPage(int code, const std::string& path) { _errorPages[code] = path; }
        void    addLocation(const LocationBlock& loc) { _locations.push_back(loc); }

        // getters
        std::string     getHost() const{ return _host; };
        int             getPort() const { return _port; };
        const std::vector<std::string>& getServerNames() const { return _serverName; }
        const std::string&  getRoot() const { return _root; };
        const std::vector<std::string>& getIndex() const { return _index; };
        size_t          getClientMaxBodySize() const { return _clientMaxBodySize;};
        const std::map<int, std::string>&               getErrorPages() const { return _errorPages; };

        //check location blocks in server to serve the correct one
        const LocationBlock* findMatchingLocation(const std::string& requestPath) const;

    private:
        std::string                 _host;
        int                         _port;
        std::string                 _root;
        std::vector<std::string>    _serverName;
        std::vector<std::string>    _index;
        std::map<int, std::string>  _errorPages;
        size_t                      _clientMaxBodySize;
        std::set<std::pair<std::string, int> > _listen;
};

#endif