#include "LocationBlock.hpp"
#include "ServerBlock.hpp"

// 1 - check que les directives ne soient pas vides (key sans value) et correctes (celles demandées dans le sujet)
// 2 - fillLocationBlock : initialise les structures Location avec les valeurs données dans le bloc Location, 
// ou avec celles du server si elles ne sont pas précisées (surcharge)

bool    ServerBlock::checkLocationBlock(const LocationBlock& location)
{
    if (location.path.empty())
    {
        std::cerr << "Config error: invalid location path: " << location.path << std::endl;
        return false; 
    }
    std::map<std::string, std::string>  errorMessages;
    errorMessages["index"] = "No index";
    errorMessages["autoindex"] = "No autoindex";
    errorMessages["allow_methods"] = "Empty allow_methods";
    errorMessages["root"] = "No root";
    errorMessages["client_max_body_size"] = "No client_max_body_size";
    errorMessages["cgi_path"] = "No cgi_path";
    errorMessages["upload_dir"] = "No upload_dir";

    std::set<std::string>   acceptedDirectives;
    for (std::map<std::string, std::string>::iterator it = errorMessages.begin(); it != errorMessages.end(); ++it){
        acceptedDirectives.insert(it->first);
    }
    for (size_t i = 0; i < location.directives.size(); ++i)
    {
        const   Directive& currentDir = location.directives[i];
        if (acceptedDirectives.find(currentDir.key) == acceptedDirectives.end()){
            std::cerr << "Config error: unknown directive: " << currentDir.key << std::endl;
            return false;
        }
        if (currentDir.value.empty())
        {
            LOG_ERR(errorMessages[currentDir.key]);
            return false;
        }
        if (currentDir.key == "allow_methods")
        {
            std::vector<std::string>    methods  = splitLine(currentDir.value, " \t");
            for (size_t j = 0; j < methods.size(); ++j){
                if (methods[j] != "GET" && methods[j] != "POST" && methods[j] != "DELETE"){
                    std::cerr << "Config error: method not allowed: " << methods[j] << std::endl;
                    return false;
                }
            }       
        }
    }
    return true;
}


bool fillLocationBlock(LocationBlock& loc, const ServerBlock& server)
{
    bool    hasIndex = false;
    std::set<std::string>   seenDirectives;

    loc.root = server.getRoot();
    loc.maxBodySize = server.getClientMaxBodySize();
    loc.index = server.getIndex();
    for (size_t i = 0; i < loc.directives.size(); ++i){
        const Directive& dir = loc.directives[i];

        if (seenDirectives.find(dir.key) != seenDirectives.end()){
            std::cerr << "Config error: duplicate directive " << dir.key << std::endl;
            return false;
        }
        seenDirectives.insert(dir.key);
        if (dir.key == "allow_methods") {
            loc.allowedMethods = splitLine(dir.value, " \t");
            if (loc.allowedMethods.empty()) {
                LOG_ERR("Config error: empty allow_methods");
                return false;
            }
            for (size_t j = 0; j < loc.allowedMethods.size(); ++j) {
                const std::string& method = loc.allowedMethods[j];
                if (method != "GET" && method != "POST" && method != "DELETE") {
                    std::cerr << "Config error: method not allowed: " << method << std::endl;
                    return false;
                }
            }
        }
        else if (dir.key == "root") {
            loc.root = dir.value;
            if (access(loc.root.c_str(), F_OK) != 0) {
                std::cerr << "Config error: cannot access root: " << loc.root << std::endl;
                return false;
            }
        }
        else if (dir.key == "index") {
            loc.index = splitLine(dir.value, " \t");
            if (loc.index.empty()) {
                LOG_ERR("Config error: empty index");
                return false;
            }
            hasIndex = true;
        }
        else if (dir.key == "autoindex")
        {
            if (dir.value == "on")
                loc.autoindex = true;
            else if (dir.value == "off")
                loc.autoindex = false;
            else {
                LOG_ERR("Config error: autoindex be on or off");
                return false;
            }
        }
        else if (dir.key == "upload_dir") {
            loc.uploadDir = dir.value;
            if (access(loc.uploadDir.c_str(), W_OK) != 0) {
                std::cerr << "Config error: cannot access upload_dir: " << loc.uploadDir << std::endl;
                return false;
            }
        }
        else if (dir.key == "cgi_path") {
            loc.cgiPath = dir.value;
            if (access(loc.cgiPath.c_str(), X_OK) != 0) {
                std::cerr << "Config error: cannot execute cgi_path: " << loc.cgiPath << std::endl;
                return false;
            }
        }
        else if (dir.key == "client_max_body_size") {
            std::string sizeStr = dir.value;
            if (sizeStr.empty()) {
                LOG_ERR("Config error: empty client_max_body_size");
                return false;
            }

            long size = std::atoi(sizeStr.c_str());
            if (!sizeStr.empty() && isdigit(sizeStr[sizeStr.length() - 1]) == 0) {
                char unit = tolower(sizeStr[sizeStr.length() - 1]);
                if (unit == 'k') size *= 1024;
                else if (unit == 'm') size *= 1024 * 1024;
                else if (unit == 'g') size *= 1024 * 1024 * 1024;
            }
            if (size <= 0) {
                LOG_ERR("Config error: invalid client_max_body_size");
                return false;
            }
            loc.maxBodySize = static_cast<size_t>(size);
        }
    }
    if (hasIndex){
        bool hasValidIndex = false;
        for (size_t i = 0; i < loc.index.size(); ++i) {
            std::string fullPath = loc.root + "/" + loc.index[i];
            if (access(fullPath.c_str(), F_OK) == 0) {
                hasValidIndex = true;
                break;
            }
        }
        if (!hasValidIndex){
            LOG_ERR("Config error: cannot access any of indexes in location block");
            return false;
        }
    }
    return true;
}
