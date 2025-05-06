#include "ParsingUtils.hpp"

std::string removeCommentsAndEndSpaces(const std::string& line)
{
    //  on enlève le commentaire de la ligne s'il y a un #
    std::string result = line;
    size_t  commentPos = result.find('#');
    if (commentPos == std::string::npos)
        return line; //pas de commentaire
    result.erase(commentPos, line.length());

    // Trim les espaces et tabulations pour retourner la chaine propre
    size_t  start = 0;
    while (start < result.length() && std::isspace(result[start]))
        start++;
    if (start == result.length())
        return "";
    size_t  end = result.length() - 1;
    while (std::isspace(line[end]))
        end--;
    return result.substr(start, end - start + 1);
}

std::string removeComments(const std::string& line)
{
    std::string result = line;
    size_t  commentPos = result.find('#');
    if (commentPos == std::string::npos)
        return line; //pas de commentaire
    result.erase(commentPos, line.length());
    return result;
}

std::vector<std::string>    splitLine(std::string line, std::string delim)
{
    std::vector<std::string>    result;

    line = removeCommentsAndEndSpaces(line);
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
