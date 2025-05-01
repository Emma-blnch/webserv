#include "ServerConfig.hpp"
#include <iostream>


void    ServerConfig::displayConfig() const {
    for (size_t i = 0; i < _servers.size(); ++i){
        std::cout << "Server Block " << i << ":\n";
        for (size_t j = 0; j < _servers[i].directives.size(); ++j){
            std::cout << "Server directive "<< j <<" (dir value) :" << _servers[i].directives[j].key << " "<< _servers[i].directives[j].value << "\n";
        }
        std::cout << "Locations :\n";
        for (size_t j = 0; j < _servers[i].locations.size(); ++j){
            std::cout << "Location "<< j << ": path = "; 
            std::cout << _servers[i].locations[j].path << "; ";
            for (size_t k = 0; k < _servers[i].locations[j].directives.size(); ++k)
            {
                std::cout << "directive "<< k << " (dir value) :" << _servers[i].locations[j].directives[k].key << " "<<  _servers[i].locations[j].directives[k].key << "\n" ; 
            }
        }
    }
}

int main() {
    ServerConfig config;

    config.parseFile("nginx.conf");
    config.displayConfig();
    
    return 0;
}