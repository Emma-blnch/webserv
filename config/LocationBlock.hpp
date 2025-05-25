#ifndef LOCATIONBLOCK_HPP
#define LOCATIONBLOCK_HPP

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

#define LOG_ERR(msg) std::cerr << "Config error\n" << msg << std::endl

class ServerBlock;

class LocationBlock
{
    public:
        LocationBlock(): autoindex(false), maxBodySize(1000000) { allowedMethods.push_back("GET"); allowedMethods.push_back("POST"); }
        std::vector<Directive>      directives;
        std::string                 path;
        std::string                 root;
        std::vector<std::string>    allowedMethods;
        std::vector<std::string>    index;
        bool                        autoindex;
        std::string                 cgiPath;

        size_t maxBodySize;
        std::string                 uploadDir;
};

bool fillLocationBlock(LocationBlock& loc, const ServerBlock& server);

#endif