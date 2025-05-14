#ifndef CONFIGUTILS_HPP
#define CONFIGUTILS_HPP

#include <cstdlib> // pour std::atoi
#include <algorithm> // pour std::transform si besoin
#include <unistd.h>
#include "LocationBlock.hpp"

class ServerInstance;
class ServerBlock;

bool fillLocationBlock(LocationBlock& loc);

#endif