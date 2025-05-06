#ifndef UTILS_HPP
 #define UTILS_HPP

#include <string> 
#include <iostream>
#include <vector>

std::string                 removeCommentsAndEndSpaces(const std::string& line);
std::vector<std::string>    splitLine(std::string line, std::string delim);

#endif