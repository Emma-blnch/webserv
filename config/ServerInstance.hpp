#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP

#include "ServerBlock.hpp"
#include "LocationBlock.hpp"

// serverBlock = parsing brut lier au fichier de config
// serverInstance = modele d'execution avec champs bien definis

class ServerInstance {
    private:
        std::string _host;              // "127.0.0.1"
        int _port;                      // 8080
        std::string _serverName;        // "localhost", "mydomain.com"
        std::string _root;              // "./www"
        std::string _index;             // "index.html"
        size_t _clientMaxBodySize;      // en octets
        std::map<int, std::string> _errorPages; // 404 → "/errors/404.html"

        std::vector<LocationBlock> _locations;

        int _socketFd;

    public:
        // Constructeur a partir du parsing
        ServerInstance() : _port(80), _clientMaxBodySize(1000000) {}

        // Getters
        const std::string& getHost() const { return _host; }
        int getPort() const { return _port; }
        const std::string& getServerName() const { return _serverName; }
        const std::string& getRoot() const { return _root; }
        const std::string& getIndex() const { return _index; }
        size_t getClientMaxBodySize() const { return _clientMaxBodySize; }
        const std::map<int, std::string>& getErrorPages() const { return _errorPages; }
        const std::vector<LocationBlock>& getLocations() const { return _locations; }
        int getSocketFd() const { return _socketFd; }

        // Setters
        void setHost(const std::string& host) { _host = host; }
        void setPort(int port) { _port = port; }
        void setServerName(const std::string& name) { _serverName = name; }
        void setRoot(const std::string& root) { _root = root; }
        void setIndex(const std::string& index) { _index = index; }
        void setClientMaxBodySize(size_t size) { _clientMaxBodySize = size; }
        void addErrorPage(int code, const std::string& path) { _errorPages[code] = path; }
        void addLocation(const LocationBlock& loc) { _locations.push_back(loc); }

        bool setupSocket(); // pour creer, bind et listen le socket (en void peut etre plutot ?)

        // trouver le bloc Location selon le chemin demander (a utiliser dans class Response)
        const LocationBlock* findMatchingLocation(const std::string& requestPath) const {
            for (size_t i = 0; i < _locations.size(); ++i) {
                if (requestPath.find(_locations[i].path) == 0)
                    return &_locations[i];
            }
            return NULL;
        }
};

#endif