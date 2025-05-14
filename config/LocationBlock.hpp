#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <cstdlib> // pour std::atoi
#include <algorithm> // pour std::transform si besoin
#include <unistd.h>
#include <iostream>

#include "ParsingUtils.hpp"
#include "Directive.hpp"

class ServerBlock;

class LocationBlock
{
    public:
        std::vector<Directive>      directives;
        std::string                 path;
        std::string                 root;
        std::vector<std::string>    allowedMethods;
        std::vector<std::string>    index;
        bool                        autoindex;

        size_t maxBodySize;

        std::string                 uploadDir; // destination des fichiers uploadés
        std::string                 cgiPath; // chemin vers script exécutable ou interpréteur
};

bool fillLocationBlock(LocationBlock& loc);