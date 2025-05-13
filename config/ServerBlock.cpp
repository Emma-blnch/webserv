#include "ServerBlock.hpp"

static bool hasListen = false;
static bool hasHost = false;
static bool hasClientMaxBodySize = false;
static bool hasServerName = false;
static bool hasRoot = false;
static bool hasIndex = false;

bool    ServerBlock::isValidClientBodySize(const Directive& directive)
{
    if (directive.value.empty())
    {
        std::cout << "Erreur config: manque client max body size ??\n";
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
            std::cout << "Erreur config: bodymaxsize < intmin ou > intmax\n";
            return false;
        }
        return true;
    }
    std::string unit = size.substr(i); // prends juste les chiffres de la string (car par ex 1M ou 1G est valide)
    if (unit == "k" || unit == "K" || unit == "m" || unit == "M" || 
        unit == "g" || unit == "G"){
            return true;
        }
    std::cout << "Erreur config: clientmaxbodysize invalide\n";
    return false;
}

bool    ServerBlock::isValidPort(const Directive& directive)
{
    if (directive.value.empty()) // port =  directive.value
    {
        std::cout << "Erreur config: listen to what ??\n";
        return false;
    }

    // Format host::port dans listen
    std::string value;
    size_t      sep = value.find(':');

    if (sep != std::string::npos){
        std::string host = value.substr(0, sep);
        std::string port = value.substr(sep + 1);
        Directive hostDirective;
        hostDirective.key = "host";
        hostDirective.value = host;
        if (!isValidHost(hostDirective))
            return false;
        _host = host;
        if (port.empty())
        {
            std::cout << "directive listen invalid: pas de port après le ':'\n";
            return false;
        }
        for (size_t i = 0; i < port.length(); ++i)
        {
            if (!std::isdigit(port[i]))
            {
                std::cout << "Erreur config: le port " << port << " n'est pas valide\n";
                return false;
            }
        }
        long   result = std::stol(port);
        if (result < INT_MIN || result > INT_MAX || result <= 0 || result > 65535)
        {
            std::cout << "Erreur config: le port " << port << " n'est pas valide\n";
            return false;
        }
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
        std::cout << "Erreur config: le port " << directive.value << " n'est pas valide\n";
        return false;
    }
    return true;
}

bool    ServerBlock::checkLocationBlock(const LocationBlock& location)
{

    // try to open location path
    if (location.path.empty())
    {
        std::cout << "Config error: invalid location path: " << location.path << std::endl;
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
                std::cout << "Config error: empty allow_methods\n";
                return false;
            }
            std::vector<std::string>    methods  = splitLine(currentDir.value, " \t");
            for (size_t j = 0; j < methods.size(); ++j){
                if (methods[j] != "GET" && methods[j] != "POST"){
                    std::cout << "Config error: method not allowed: " << methods[j] << std::endl;
                    return false;
                }
            }       
        }
        else if (currentDir.key == "root"){
            hasRoot = true;
            if (currentDir.value.empty())
            {
                std::cout << "Config errror: no root\n";
                return false;
            }
        }
        else if (currentDir.key == "index"){
            hasIndex = true;
            if (currentDir.value.empty()){
                std::cout << "Config error: no index\n";
                return false;
            }
        }
    }
    return (hasRoot && hasAllowMethods && hasIndex);
}

bool    ServerBlock::isValidHost(const Directive& directive)
{
    if (directive.value.empty()){
        std::cout << "token host sans host\n";
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
        std::cout << "Hôte (IP) invalide: "<< host << std::endl;
        return false;
    }
    for (size_t i = 0; i < 4; ++i){
        for (size_t j = 0; j < bytes[i].size(); j++){
            if (!std::isdigit(bytes[i][j])){
                std::cout << "Hôte (IP) invalide: " << host << std::endl;
                return false;
            }
        }
        int byteValue = std::stoi(bytes[i]);
        if (byteValue < 0 || byteValue > 255){
            std::cout << "Hôte (IP) invalide: " << host << std::endl;   
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
    else if (directive.key == "host") {
        if (hasHost) {
            return true;
        }
        hasHost = true;
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
    hasHost = false;
    hasClientMaxBodySize = false;
    hasServerName = false;
    hasRoot = false;
    hasIndex = false;
}
// TO DO doublons à vérifier (check le booléen)
bool    ServerBlock::checkServerBlock(const ServerBlock& server)
{
    for (size_t i = 0; i < server.directives.size(); ++i)
    {
        const Directive& currentDir = server.directives[i];
        if (isDoubleDirective(currentDir))
        {
            std::cout << "Erreur config: directive " << currentDir.key << " déjà présente\n";
            return false;
        }
        if (currentDir.key == "listen")
        {
            if (!isValidPort(currentDir))
                return false;
            _port = std::stoi(currentDir.value);
        }
        else if (currentDir.key == "host"){
            if (!isValidHost(currentDir))
                return false;
            _host = currentDir.value;
        }
        else if (currentDir.key == "client_max_body_size") 
        {
            if (!isValidClientBodySize(currentDir))
                return false;
            _clientMaxBodySize = currentDir.value;
        }
        else if (currentDir.key == "server_name")
        {
            if (currentDir.value.empty())
            {
                std::cout << "token server_name sans name\n";
                return false;
            }
            _serverName = currentDir.value;
        }
        else if (currentDir.key == "error_page")
        {
            if (currentDir.value.empty())
            {
                std::cout << "Erreur config: manque page(s) d'erreur\n";
                return false;
            }
        }
        else
        {
            std::cout << "Directive non autorisée: " << server.directives[i].key << std::endl;
            return false;
        }
    }
    for (size_t i = 0; i < server.locations.size(); ++i){
        if (!checkLocationBlock(server.locations[i]))
            return false;
    }
    unsetDoubleDirective();
    return true;
}
