#include "ServerBlock.hpp"
#include <fcntl.h>
#include <string>

const LocationBlock* ServerBlock::findMatchingLocation(const std::string& requestPath) const {
    for (size_t i = 0; i < _locations.size(); ++i) {
        if (pathMatches(_locations[i].path, requestPath))
            return &_locations[i];
    }
    return NULL;
}

bool    ServerBlock::isValidClientBodySize(const Directive& directive)
{
    if (directive.value.empty())
    {
        LOG_ERR("Missing clientmaxbodysize");
        return false;
    }
    std::string size = directive.value;
    size_t  i = 0;
    while (i < size.length() && std::isdigit(size[i]))
        i++;
    if (i == 0) // no digit
        return false;
    if (i == size.length())
    {
        long    result = asLong(size);
        if (result < INT_MIN ||  result > INT_MAX)
        {
            LOG_ERR("clientmaxbodysize < intmin ou > intmax");
            return false;
        }
        return true;
    }
    std::string unit = size.substr(i);
    if (unit == "k" || unit == "K" || unit == "m" || unit == "M" || 
        unit == "g" || unit == "G"){
            return true;
        }
    LOG_ERR("Invalid clientmaxbodysize");
    return false;
}

bool    ServerBlock::validateListen(const Directive& directive)
{
    if (directive.value.empty())
    {
        LOG_ERR("Empty listen");
        return false;
    }
    //  host::port format
    std::string value = directive.value;
    size_t      sep = value.find(':');
    std::string host = "0.0.0.0";

    if (sep != std::string::npos){
        host = value.substr(0, sep);
        std::string port = value.substr(sep + 1);
        Directive hostDirective;
        hostDirective.key = "host";
        hostDirective.value = host;
        if (!isValidHost(hostDirective))
            return false;
        if (port.empty())
        {
            LOG_ERR("Port is missing");
            return false;
        }
        for (size_t i = 0; i < port.length(); ++i)
        {
            if (!std::isdigit(port[i]))
            {
                std::cerr << "Config error: " << port << ": invalid port\n";
                return false;
            }
        }
        long   result = asLong(port);
        if (result < INT_MIN || result > INT_MAX || result <= 0 || result > 65535)
        {
            std::cerr << "Config error: " << port << ": invalid port\n";
            return false;
        }
        std::pair<std::string, int> entry = std::make_pair(host, static_cast<int>(result));
        if (_listen.find(entry) != _listen.end()) {
            std::cerr << "Config error: listen " << host << ":" << static_cast<int>(result) << " is already in server\n";
            return false;
        }
        else {
            _listen.insert(entry);
        }
        setPort(static_cast<int>(result));
        setHost(host);
        return true;
    }
    // listen + port format
    size_t  i = 0;
    while(i < directive.value.length())
    {
        if (!std::isdigit(directive.value[i]))
            return false;
        i++;
    }
    long    result = asLong(directive.value);
    if (result < INT_MIN || result > INT_MAX || result <= 0 || result > 65535)
    {
        std::cerr << "Config error: " << directive.value << " invalid port\n";
        return false;
    }
    std::pair<std::string, int> entry = std::make_pair(host, static_cast<int>(result));
    if (_listen.find(entry) != _listen.end()) {
        std::cerr << "Config error: listen " << host << ":" <<  static_cast<int>(result) << " is already in server\n";
    }
    else {
        _listen.insert(entry);
    }
    setPort(static_cast<int>(result));
    setHost(host);
    return true;
}


bool    ServerBlock::isValidHost(const Directive& directive)
{
    if (directive.value.empty()){
        LOG_ERR("Host is missing");
        return false;
    }
    if (directive.value == "localhost" || directive.value == "0.0.0.0"){
        return true;
    }
    // stock bytes in host
    std::string host = directive.value;
    std::string currentByte = "";
    std::vector<std::string>    bytes;
    for (size_t i = 0; i < host.size(); ++i){
        if (host[i] == '.'){
            bytes.push_back(currentByte);
            currentByte = "";
        } else {
            currentByte += host[i];
        }
    }
    bytes.push_back(currentByte);
    if(bytes.size() != 4){
        std::cerr << "Config error: Host (IP format) "<< host << " is invalid\n";
        return false;
    }
    for (size_t i = 0; i < 4; ++i){
        for (size_t j = 0; j < bytes[i].size(); j++){
            if (!std::isdigit(bytes[i][j])){
                std::cerr << "Config error: Host (IP format) "<< host << " is invalid\n";
                return false;
            }
        }
        int byteValue = asInt(bytes[i]);
        if (byteValue < 0 || byteValue > 255){
            std::cerr << "Config error: Host (IP format) "<< host << " is invalid\n";
            return false;
        }
    }
    return true;
}

const std::map<int, std::string>& errorStatus() {
    static std::map<int, std::string> m;
    if (m.empty()) {
        m[200] = "OK";
        m[201] = "Created";
        m[204] = "No Content";
        m[400] = "Bad Request";
        m[403] = "Forbidden";
        m[404] = "Not Found";
        m[405] = "Method Not Allowed";
        m[413] = "Payload Too Large";
        m[414] = "URI Too Long";
        m[500] = "Internal Server Error";
        m[501] = "Not Implemented";
        m[505] = "HTTP Version Not Supported";
    }
    return m;
}

bool    ServerBlock::checkServerBlock()
{
    std::set<std::string>       seenDirectives;
    std::vector<std::string>           indexes;
    bool    hasIndex = false;
    bool    hasRoot = false;
    bool    hasServerName = false;

    for (size_t i = 0; i < directives.size(); ++i)
    {
        const Directive& currentDir = directives[i];

        if (seenDirectives.find(currentDir.key) != seenDirectives.end())
        {
            std::cerr << "Config error: duplicate directive: " << currentDir.key << std::endl;
            return false;
        }
        if (currentDir.key == "listen")
        {
            if (!validateListen(currentDir))
                return false;
        }
        else if (currentDir.key == "root"){
            if (currentDir.value.empty())
            {
                LOG_ERR("No root");
                return false;
            }
            hasRoot = true;
            std::string path = currentDir.value;
            int fd = open(path.c_str(), O_DIRECTORY);
            if (fd < 0) {
                LOG_ERR("Root is not a directory");
                return false;
            }
            close(fd);
            setRoot(path);
        }
        else if (currentDir.key == "index"){
            if (currentDir.value.empty()){
                LOG_ERR("No index");
                return false;
            }
            hasIndex = true;
            indexes = splitLine(currentDir.value, " \t");
        }
        else if (currentDir.key == "client_max_body_size") 
        {
            if (!isValidClientBodySize(currentDir))
                return false;
            std::string sizeStr = currentDir.value;
            long size = std::atoi(sizeStr.c_str());
            if (!sizeStr.empty() && isdigit(sizeStr[sizeStr.length() - 1]) == 0) {
                char unit = tolower(sizeStr[sizeStr.length() - 1]);
                if (unit == 'k') size *= 1024;
                else if (unit == 'm') size *= 1024 * 1024;
                else if (unit == 'g') size *= 1024 * 1024 * 1024;
            }
            setClientMaxBodySize(static_cast<size_t>(size));
        }
        else if (currentDir.key == "server_name")
        {
            if (hasServerName) { // pour éviter 2 server_name dans un meme bloc server ? d'ailleurs il faudrait ca pour tout non ?
                LOG_ERR("Multiple server_name directives in one server block");
                return false;
            }
            if (currentDir.value.empty())
            {
                LOG_ERR("Empty server_name");
                return false;
            }
            std::vector<std::string> names = splitLine(currentDir.value, " \t");
            std::set<std::string> localSet;
            for (size_t k = 0; k < names.size(); ++k) {
                if (localSet.count(names[k])) {
                    std::cerr << "Duplicate server_name '" << names[k] << "' in the same server block." << std::endl;
                    return false;
                }
                localSet.insert(names[k]);
            }
            hasServerName = true;
            setServerNames(names);
        }
        else if (currentDir.key == "error_page")
        {
            if (currentDir.value.empty())
            {
                LOG_ERR("Missing error page(s)");
                return false;
            }
            std::vector<std::string> errorPart = splitLine(currentDir.value, " \t");
            if (errorPart.size() >= 2) {
                int code = std::atoi(errorPart[0].c_str());
                std::map<int, std::string>::const_iterator it = errorStatus().find(code);
                if (it == errorStatus().end()) {
                    LOG_ERR("Invalid error code");
                    return false;
                }
                std::string path = errorPart[1];
                if (access(path.c_str(), F_OK) != 0) {
                    std::cerr << "Config error: cannot found error page " << path << std::endl;
                    return false;
                }
                addErrorPage(code, path);
            }
            else {
                LOG_ERR("Error_page requires at least a code and a path");
                return false;
            }
        }
        else
        {
            std::cerr << "Config error: directive " << directives[i].key << " is not allowed\n";
            return false;
        }
    }
    if (hasIndex){
        bool    hasValidIndex = false;
        for (size_t i = 0; i < indexes.size(); ++i){
            std::string path2 = _root + "/" + indexes[i];
            if (access(path2.c_str(), F_OK) == 0){
                hasValidIndex = true;
                break;
            }
        }
        if (!hasValidIndex){
            LOG_ERR("Cannot access any of indexes in server block");
            return false;
        }
        setIndexes(indexes);
    }
    for (size_t i = 0; i < _locations.size(); ++i){
        if (!checkLocationBlock(_locations.back()))
            return false;
        if (!fillLocationBlock(_locations.back(), *this)) {
            LOG_ERR("Invalid location block");
            return false;
        }
    }
    if (!hasRoot){
        LOG_ERR("No root in server");
        return false;
    }
    return true;
}
