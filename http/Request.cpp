#include "Request.hpp"
#include "utils.hpp"

void Request::parseRequestLine(const std::string& line) {
    if (line.empty())
        throw std::runtime_error("Empty request line");
    if (line.size() > 8192) // valeur de sécurité classique dans les serveurs web pour limiter la taille maximale autorisée d'une ligne HTTP
        throw std::runtime_error("Request line too long");
    
    size_t start = 0;
    size_t end = line.find(' ');
    int i = 0;

    while (end != std::string::npos && i < 3) {
        std::string token = line.substr(start, end - start);
        if (token.empty())
            throw std::runtime_error("Malformed request line: empty token");
        if (i == 0)
        {
            _method = token;
            if (_method != "GET" && _method != "POST" && _method != "DELETE") // changer pour renovyer 405 et pas catch 400
                throw std::runtime_error("405: Unsupported HTTP method: " + _method);
            // faire en sorte que ca renvoi erreur 405 "Method Not Allowed" -> setStatus et setBody
        }
        else if (i == 1) {
            _path = token;
            // separation path et query string
            size_t pos = _path.find('?');
            if (pos != std::string::npos) {
                _query = _path.substr(pos + 1);
                _path = _path.substr(0, pos);
            }
            else
                _query = "";
        }
        _path = normalizePath(_path); // éviter accès à fichiers protégés
        start = end + 1;
        end = line.find(' ', start);
        i++;
    }
    if (i != 2)
        throw std::runtime_error("Malformed request line: missing parts");
    _version = line.substr(start);
    if (_version.substr(0, 5) != "HTTP/")
        throw std::runtime_error("Invalid HTTP version");
    if (_version != "HTTP/1.1")
        throw std::runtime_error("Unsupported HTTP version: " + _version);
    // Sécurité sur _path
    if (_path.find("..") != std::string::npos || _path.find('\0') != std::string::npos) {
        throw std::runtime_error("Unsafe request path (../ or null byte)");
    }
    if (_path.size() > 2048) {
        throw std::runtime_error("Request path too long");
    }
}

void Request::parseHeaderLine(const std::string& line) {
    size_t colon = line.find(":");

    if (colon == std::string::npos)
        throw std::runtime_error("Malformed header line: missing colon");
    std::string key = line.substr(0, colon);
    std::string value;
    std::string space = line.substr(colon + 1);
    // Trim début de value (espace/tabs)
    size_t firstNonSpace = space.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos)
        value = space.substr(firstNonSpace);
    else
        value = line.substr(colon + 1);
    key = toLower(key);
    _header[key] = value;
}

void Request::parsePostRequest(const std::string& line) {
    std::string contentLenStr = getHeader("content-length");
    if (contentLenStr.empty())
        throw std::runtime_error("Missing content-length in POST request");
    
    std::istringstream lenStream(contentLenStr);
    int contentLen = 0;
    lenStream >> contentLen;
    if (lenStream.fail() || contentLen < 0)
        throw std::runtime_error("Invalid content-length value");

    if ((int)line.size() < contentLen)
        throw std::runtime_error("Incomplete request body");
    _body = line.substr(0, contentLen);
}

void Request::parseRawRequest(const std::string& req) {
    // trouver fin des headers
    size_t headersEnd = req.find("\r\n\r\n");
    if (headersEnd == std::string::npos)
        throw std::runtime_error("Invalid HTTP request: missing header/body separator");
    
    // separer headers et body
    std::string headersPart = req.substr(0, headersEnd);
    std::string bodyPart = req.substr(headersEnd + 4);

    // lire headers ligne par ligne
    std::istringstream stream(headersPart);
    std::string line;
    bool isFirstLine = true;

    while(std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r') // vire manuellement le '\r' car lignes HTTP finissent par '\r\n' mais getline supp que '\n'
            line.erase(line.size() - 1);
        
        if (isFirstLine) {
            parseRequestLine(line);
            isFirstLine = false;
        }
        else {
            parseHeaderLine(line);
        }
    }
    if (_method == "POST") {
        parsePostRequest(bodyPart);
    }

    // verifier qu'on a bien un host
    if (getHeader("host").empty())
        throw std::runtime_error("Missing required Host header");
}

std::string Request::getMethod() const {
    return _method;
}

std::string Request::getPath() const {
    return _path;
}

std::string Request::getVersion() const {
    return _version;
}

std::string Request::getHeader(const std::string& key) const {
    std::string lowerKey = toLower(key);
    if (_header.find(lowerKey) != _header.end())
        return _header.at(lowerKey);
    return "";
}

std::string Request::getBody() const {
    return _body;
}

std::string Request::getQuery() const {
    return _query;
}