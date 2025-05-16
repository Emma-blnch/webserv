#include "ServerBlock.hpp"
#include <fcntl.h>

static bool hasListen = false;
static bool hasClientMaxBodySize = false;
static bool hasServerName = false;
static bool hasRoot = false;
static bool hasIndex = false; 

bool    ServerBlock::isValidClientBodySize(const Directive& directive)
{
    if (directive.value.empty())
    {
        std::cerr << "Erreur config: manque client max body size\n";
        return false;
    }
    std::string size = directive.value;
    size_t  i = 0;
    while (i < size.length() && std::isdigit(size[i]))
        i++;
    if (i == 0) // aucun chiffre
        return false;
    if (i == size.length()) // que des chiffres, juste à vérifier l'overflow
    {
        long    result = std::stol(size);
        if (result < INT_MIN ||  result > INT_MAX)
        {
            LOG_ERR("Bodymaxsize < intmin ou > intmax");
            return false;
        }
        return true;
    }
    std::string unit = size.substr(i); // prends juste les chiffres de la string (car par ex 1M ou 1G est valide)
    if (unit == "k" || unit == "K" || unit == "m" || unit == "M" || 
        unit == "g" || unit == "G"){
            return true;
        }
    LOG_ERR("Clientmaxbodysize invalide");
    return false;
}

bool    ServerBlock::validateListen(const Directive& directive)
{
    if (directive.value.empty()) // port =  directive.value
    {
        LOG_ERR("Listen to what ??");
        return false;
    }

    // Format host::port dans listen
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
            LOG_ERR("Directive listen invalid: pas de port après le ':'");
            return false;
        }
        for (size_t i = 0; i < port.length(); ++i)
        {
            if (!std::isdigit(port[i]))
            {
                std::cerr << "Le port " << port << " n'est pas valide\n";
                return false;
            }
        }
        long   result = std::stol(port);
        if (result < INT_MIN || result > INT_MAX || result <= 0 || result > 65535)
        {
            std::cerr << "Le port " << port << " n'est pas valide\n";
            return false;
        }
        std::pair<std::string, int> entry = std::make_pair(host, static_cast<int>(result));
        if (_listen.find(entry) != _listen.end()) {
            std::cerr << "Erreur config: listen " << host << ":" << static_cast<int>(result) << " dupliqué\n";
        }
        else {
            _listen.insert(entry);
        }
        setPort(static_cast<int>(result));
        setHost(host);
        return true;
    }
    // sinon (juste listen port)
    size_t  i = 0;
    while(i < directive.value.length())
    {
        if (!std::isdigit(directive.value[i]))
            return false;
        i++;
    }
    long    result = std::stol(directive.value);
    if (result < INT_MIN || result > INT_MAX || result <= 0 || result > 65535)
    {
        std::cerr << "Erreur config: le port " << directive.value << " n'est pas valide\n";
        return false;
    }
    std::pair<std::string, int> entry = std::make_pair(host, static_cast<int>(result));
    if (_listen.find(entry) != _listen.end()) {
        std::cerr << "Erreur config: listen " << host << ":" <<  static_cast<int>(result) << " dupliqué\n";
    }
    else {
        _listen.insert(entry);
    }
    setPort(static_cast<int>(result));
    setHost(host);
    return true;
}

bool    ServerBlock::checkLocationBlock(const LocationBlock& location)
{

    // try to open location path
    if (location.path.empty())
    {
        std::cerr << "Config error: invalid location path: " << location.path << std::endl;
        return false; 
    }
    
    bool    hasAllowMethods = false;
    bool    hasRoot = false;
    bool    hasIndex = false;

    for (size_t i = 0; i < location.directives.size(); ++i)
    {
        const   Directive& currentDir = location.directives[i];

        if (currentDir.key == "allow_methods")
        {
            hasAllowMethods = true;
            if (currentDir.value.empty()){
                LOG_ERR("Empty allow_methods");
                return false;
            }
            std::vector<std::string>    methods  = splitLine(currentDir.value, " \t");
            for (size_t j = 0; j < methods.size(); ++j){
                if (methods[j] != "GET" && methods[j] != "POST" && methods[j] != "DELETE"){
                    std::cerr << "Config error: method not allowed: " << methods[j] << std::endl;
                    return false;
                }
            }       
        }
        else if (currentDir.key == "root"){
            hasRoot = true;
            if (currentDir.value.empty())
            {
                LOG_ERR("No root");
                return false;
            }
        }
        else if (currentDir.key == "index"){
            hasIndex = true;
            if (currentDir.value.empty()){
                LOG_ERR("No index");
                return false;
            }
        }
    }
    return (hasRoot && hasAllowMethods && hasIndex);
}

bool    ServerBlock::isValidHost(const Directive& directive)
{
    if (directive.value.empty()){
        LOG_ERR("host is missing");
        return false;
    }
    if (directive.value == "localhost" || directive.value == "0.0.0.0"){
        return true;
    }
    // stocke les octets de l'hôte
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

    //check la validité de l'hôte
    if(bytes.size() != 4){
        std::cerr << "Hôte (IP) invalide: "<< host << std::endl;
        return false;
    }
    for (size_t i = 0; i < 4; ++i){
        for (size_t j = 0; j < bytes[i].size(); j++){
            if (!std::isdigit(bytes[i][j])){
                std::cerr << "Hôte (IP) invalide: " << host << std::endl;
                return false;
            }
        }
        int byteValue = std::stoi(bytes[i]);
        if (byteValue < 0 || byteValue > 255){
            std::cerr << "Hôte (IP) invalide: " << host << std::endl;   
            return false;
        }
    }
    return true;
}
bool    ServerBlock::isDoubleDirective(const Directive& directive)
{
    if (directive.key == "listen"){
        if (hasListen){
            return true;
        }
        hasListen = true;
    }
    else if (directive.key == "client_max_body_size") {
        if (hasClientMaxBodySize) {
            return true;
        }
        hasClientMaxBodySize = true;
    }
    else if (directive.key == "server_name") {
        if (hasServerName) {
            return true;
        }
        hasServerName = true;
    }
    else if (directive.key == "root") {
        if (hasRoot) {
            return true;
        }
        hasRoot = true;
    }
    else if (directive.key == "index") {
        if (hasIndex) {
            return true;
        }
        hasIndex = true;
    }
    return false;
}

void    ServerBlock::unsetDoubleDirective()
{
    hasListen = false;
    hasClientMaxBodySize = false;
    hasServerName = false;
    hasRoot = false;
    hasIndex = false;
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

bool    ServerBlock::checkServerBlock(ServerBlock& server)
{
    for (size_t i = 0; i < server.directives.size(); ++i)
    {
        const Directive& currentDir = server.directives[i];
        if (isDoubleDirective(currentDir))
        {
            std::cerr << "Erreur config: directive " << currentDir.key << " déjà présente\n";
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
            std::string path = currentDir.value;
            int fd = open(path.c_str(), O_DIRECTORY);
            if (fd < 0) {
                LOG_ERR("Ce n'est pas un dossier");
                return false;
            }
            close(fd);
            if (path != "www" && path != "./www") {
                LOG_ERR("Mauvais dossier root");
                return false;
            }
            setRoot(path);
        }
        else if (currentDir.key == "index"){
            if (currentDir.value.empty()){
                LOG_ERR("Config error: no index");
                return false;
            }
            std::string path2 = _root + "/" + currentDir.value;
            if (access(path2.c_str(), F_OK) != 0) {
                std::cerr << "Erreur config: cannot access index " << server.getIndex() << std::endl;
                return false;
            }
            setIndex(currentDir.value);
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
            if (currentDir.value.empty())
            {
                LOG_ERR("Token server_name sans name");
                return false;
            }
            _serverName = currentDir.value;
            // std::set<std::string> names = splitLineSet(currentDir.value, " \t");
            // if (names.empty())
            // {
            //     LOG_ERR("Server_name vide");
            //     return false;
            // }
            // _serverNames.insert(names.begin(), names.end());
        }
        else if (currentDir.key == "error_page")
        {
            if (currentDir.value.empty())
            {
                LOG_ERR("Manque page(s) d'erreur");
                return false;
            }
            std::vector<std::string> errorPart = splitLine(currentDir.value, " \t");
            if (errorPart.size() >= 2) {
                int code = std::atoi(errorPart[0].c_str());
                std::map<int, std::string>::const_iterator it = errorStatus().find(code);
                if (it == errorStatus().end()) {
                    LOG_ERR("Code d'erreur invalide");
                    return false;
                }
                std::string path = errorPart[1];
                if (access(path.c_str(), F_OK) != 0) {
                    std::cerr << "Erreur config: page d'erreur " << path << " introuvable\n";
                    return false;
                }
                addErrorPage(code, path);
            }
            else {
                LOG_ERR("Mauvais format error_pages");
                return false;
            }
        }
        else
        {
            std::cerr << "Erreur config: directive non autorisée: " << server.directives[i].key << std::endl;
            return false;
        }
    }
    for (size_t i = 0; i < server._locations.size(); ++i){
        _locations.push_back(server._locations[i]);
        if (!checkLocationBlock(_locations.back()))
            return false;
        if (!fillLocationBlock(_locations.back())) {
            LOG_ERR("Remplissage du bloc location échoué");
            return false;
        }
    }
    unsetDoubleDirective();
    return true;
}
