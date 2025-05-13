#include "ConfigFile.hpp"
#include <iostream>


void    ConfigFile::displayConfig() const {
    for (size_t i = 0; i < _servers.size(); ++i){
        std::cout << "Server n°" << i << ":\n\n";
        for (size_t j = 0; j < _servers[i].directives.size(); ++j){
            std::cout << "s_dir "<< j << ": " << _servers[i].directives[j].key << " " << _servers[i].directives[j].value << "\n";
        }
        if (_servers[i].locations.size())
            std::cout << "\n";
        for (size_t j = 0; j < _servers[i].locations.size(); ++j){
            std::cout << "s_location n°" << i << ":\n";
            for (size_t k = 0; k < _servers[i].locations[j].directives.size(); ++k){
                std::cout << "l_dir " << k <<": " << _servers[i].locations[j].directives[k].key << " " << _servers[i].locations[j].directives[k].value << "\n";
            }
        }
        std::cout << "\n\n";
    }
}

void    splitTester(std::string line) {
    std::vector<std::string> array = splitLine(line, " \n\r");
    for (std::vector<std::string>::const_iterator i = array.begin(); i != array.end(); ++i)
        std::cout << *i << std::endl;
}

int main(int ac, char **argv) {
    ConfigFile config;

    if (ac != 2)
        return 0;
    std::string conf = argv[1];
    if (!config.parseConfigFile(conf))
        return 0;
    // std::string test = "     listen 8080;       #jeij f enfien fn enrfner jfio   \n";
    // std::cout << removeCommentsAndEndSpaces(test) << std::endl;
    config.displayConfig();
    // splitTester("         bonjour TOUT D   le778 monde  \n");

    return 0;
}