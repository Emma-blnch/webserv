#ifndef PARSINGUTILS_HPP
#define PARSINGUTILS_HPP

#include <string> 
#include <iostream>
#include <vector>
#include <set>
#include <sstream> // int,long conversions

std::string                 removeCommentsAndEndSpaces(const std::string& line);
std::vector<std::string>    splitLine(std::string line, std::string delim);
std::set<std::string>    splitLineSet(std::string line, std::string delim);

long asLong(const std::string& str);
int asInt(const std::string& str);


bool pathMatches(const std::string& locPath, const std::string& reqPath);


#endif