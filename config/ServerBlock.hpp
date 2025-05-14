#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <limits.h>
#include <iostream>
#include "ParsingUtils.hpp"
#include "LocationBlock.hpp"

// struct Listen
// {
//     std::string fullListenField; // 127.0.1.3:80
//     std::string host; // 127.0.1.3
//     unsigned int    port; // 80
// };

class ServerBlock 
{
    public:
        ServerBlock(): _host("0.0.0.0"), _port(80), _clientMaxBodySize("1M") {};
        std::vector<Directive>      directives;
        std::vector<LocationBlock>  locations;

        std::string                 getHost() const{ return _host; };
        int getPort() const { return _port; };
        const std::string& getServerName() const { return _serverName; };
        const std::string& getRoot() const { return _root; };
        const std::string& getIndex() const { return _index; };
        const std::string& getClientMaxBodySize() const { return _clientMaxBodySize;};
        const std::map<int, std::string>& getErrorPages() const { return _errorPages; };

        bool    checkServerBlock(const ServerBlock& server);
        bool    checkLocationBlock(const LocationBlock& location);

        bool    isValidPort(const Directive& directive);
        bool    isValidHost(const Directive& directive);
        bool    isValidClientBodySize(const Directive& directive);

        bool    isDoubleDirective(const Directive& directive);
        void    unsetDoubleDirective();

        // Listen&                     listen;

    private:
        std::string                 _host;
        int                         _port;
        std::string                 _root;
        std::string                 _serverName;
        std::string                 _index;
        std::map<int, std::string>  _errorPages;
        std::string                 _clientMaxBodySize;
};