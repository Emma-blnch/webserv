#ifndef UTILS_HPP
#define UTILS_HPP

#include <map>
#include <string>
#include <sstream>

class LocationBlock;

const std::map<int, std::string>& getValidStatus();
const std::map<std::string, std::string>& getValidMimeTypes();

std::string toLower(const std::string& s);

std::string normalizePath(const std::string& path);

bool isMethodAllowed(const LocationBlock* location, const std::string& method);

#endif