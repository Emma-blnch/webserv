#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include "ParsingUtils.hpp"
#include "Directive.hpp"

class LocationBlock
{
    public:
        std::vector<Directive>      directives;
        std::string                 path;
        std::string                 root;
        std::vector<std::string>    allowedMethods;
        std::vector<std::string>    index;
        bool                        autoindex;


        std::string                 uploadDir; // destination des fichiers uploadés
        std::string                 cgiPath; // chemin vers script exécutable ou interpréteur
};