#include "Response.hpp"

// pour load nos pages d'erreurs html perso
void Response::loadErrorPageIfNeeded() {
    if (_status == 204 || _status == 304)
        return;

    std::map<int, std::string>::const_iterator it = _errorPages.find(_status);
    if (it != _errorPages.end()) {
        std::string path = it->second;
        std::ifstream file(path.c_str());
        if (file) {
            std::cout << "Page loaded : " << path << std::endl;
            std::ostringstream content;
            content << file.rdbuf();
            file.close();
            setBody(content.str());
            setHeader("Content-Type", "text/html");
        } else
            setBody(getValidStatus().at(_status)); // fallback si le fichier HTML d'erreur n'existe pas
    }
}

// setters
void Response::setStatus(int code) {
    std::map<int, std::string>::const_iterator it = getValidStatus().find(code);
    if (it == getValidStatus().end()) {
        std::ostringstream oss;
        oss << "Invalid HTTP status code: " << code;
        throw std::runtime_error(oss.str());
    }
    _status = code;
    _stateMsg = it->second;

    loadErrorPageIfNeeded();
}

void Response::setHeader(const std::string& key, const std::string& value) {
    if (key.empty())
        throw std::runtime_error("Header key cannot be empty");

    std::string lowerKey = toLower(key);
    if (lowerKey == "content-length")
        throw std::runtime_error("Use setBody() to set Content-Length automatically");

    _headers[lowerKey] = value;
}

void Response::setBody(const std::string& body) {
    if (_status == 204 || _status == 304)  // ces codes ne doivent pas avoir de body
        throw std::runtime_error("This response status must not have a body");
    _body = body;

    std::ostringstream oss;
    oss << _body.size();
    _headers["content-length"] = oss.str();
    if (_headers.find("content-type") == _headers.end())
        _headers["content-type"] = "text/plain"; // si pas de Content-Type défini, on met text/plain par défaut
}

void Response::setErrorPages(const std::map<int, std::string>& pages) {
    _errorPages = pages;
}

// préparer la réponse selon méthode
void Response::buildFromRequest(const Request& req, const ServerBlock& server) {
    std::string path = req.getPath();
    const LocationBlock* location = server.findMatchingLocation(req.getPath());

    if (path.find("/cgi-bin/") == 0) {
        handleCGI(req, location, server);
    }
    else if (req.getMethod() == "GET")
        handleGET(req, server, location);
    else if (req.getMethod() == "POST")
        handlePOST(req, server, location);
    else if (req.getMethod() == "DELETE")
        handleDELETE(req, server, location);
    else {
        setStatus(405);
        setBody("Method Not Allowed");
    }
}

// réponse renvoyée
std::string Response::returnResponse() const {
    std::ostringstream response;
    // 1. Status line
    response << "HTTP/1.1 " << _status << " " << _stateMsg << "\r\n";
    // 2. Headers
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    // 2.2 Expliciter fermeture de la connexion
    response << "Connection: close\r\n";
    // 3. Ligne vide obligatoire
    response << "\r\n";
    // 4. Body (sauf si code l’interdit)
    if (!(_status == 204 || _status == 304)) {
        response << _body;
    }
    return response.str();
}
