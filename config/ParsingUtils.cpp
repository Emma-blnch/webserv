#include "ParsingUtils.hpp"

std::string removeEndSpaces(const std::string& line){
    size_t  start = 0;
    while (start < line.length() && std::isspace(line[start]))
        start++;
    if (start == line.length())
        return "";
    size_t  end = line.length() - 1;
    while (std::isspace(line[end]))
        end--;
    return line.substr(start, end - start + 1);
}

std::vector<std::string>    splitLine(std::string line, std::string delim){
    std::vector<std::string>    result;

    line = removeEndSpaces(line);
    size_t  i = 0;
    std::string token;

    while (i < line.length()){
        if (delim.find(line[i]) != std::string::npos){
            if (!token.empty()){
                result.push_back(token);
                token = "";
            }
        }
        else
            token += line[i];
        i++;
    }
    if (!token.empty()){
        result.push_back(token);
    }
    return (result);
}
