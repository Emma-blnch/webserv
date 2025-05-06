#ifndef UTILS_HPP
#define UTILS_HPP

#include <map>
#include <string>

const std::map<int, std::string>& getValidStatus();
const std::map<std::string, std::string>& getValidMimeTypes();

std::string toLower(const std::string& s);

#endif