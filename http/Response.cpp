#include "Response.hpp"
#include "Request.hpp"
#include "utils.hpp"

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

// build response for each method :

// GET
std::string Response::getExtension(const std::string& path) {
    size_t point = path.rfind(".");
    if (point == std::string::npos)
        return "";
    return path.substr(point);
}

std::string Response::guessContentType(const std::string& ext) {
    std::map<std::string, std::string>::const_iterator it = getValidMimeTypes().find(ext);
    if (it != getValidMimeTypes().end())
        return it->second;
    return "application/octet-stream"; // type MIME par défaut pour contenu binaire inconnu
}

void Response::handleGET(const Request& req) {
    // Traduire l'URI en chemin réel
    std::string path = "." + req.getPath();

    // Vérifier que le fichier existe
    if (access(path.c_str(), F_OK) != 0) {
        setStatus(404);
        setBody("404 Not Found");
        return;
    }
    // Vérifier que je peux le lire
    if (access(path.c_str(), R_OK) != 0) {
        setStatus(403);
        setBody("403 Forbidden");
        return;
    }
    // Vérifier que j'arrive à bien lire tout le fichier
    std::ifstream file(path.c_str());
    if (!file) {
        setStatus(500);
        setBody("500 Internal Server Error");
        return;
    }
    // Sinon, lire tout le contenu pour le mettre dans le body
    std::ostringstream content;
    content << file.rdbuf();
    file.close();

    // Déterminer le type MIME (Content-Type)
    std::string ext = getExtension(path);   // -> ".html"
    std::string mime = guessContentType(ext); // -> "text/html"

    // Préparer la réponse
    setStatus(200);
    setBody(content.str());
    setHeader("Content-Type", mime);
}

// POST
void Response::handlePOST(const Request& req) { // Envoyer des données au serveur
    // Vérifier la présence d’un body
    std::string body = req.getBody();

    // S’assurer qu’il est cohérent avec Content-Length
    std::string contentLenStr = req.getHeader("Content-Length");
    if (contentLenStr.empty())
    {
        setStatus(411);
        setBody("411 Length Required");
        return;
    }
    std::istringstream iss(contentLenStr);
    size_t expectedLength;
    iss >> expectedLength;
    if (iss.fail() || expectedLength != body.size()) {
        setStatus(400);
        setBody("400 Bad Request: Incorrect body size");
        return;
    }

    // Traiter le contenu
    setBody("POST received: " + body);

    // Préparer la réponse
    setStatus(200);
    setHeader("Content-Type", "text/plain");
}

// DELETE
void Response::handleDELETE(const Request& req) { // Supprimer la ressource
    // Traduire l’URI en chemin réel
    std::string path = "." + req.getPath();
    // Extraire le dossier
    std::string dir = path.substr(0, path.find_last_of('/'));
    if (dir.empty())
        dir = ".";

    // Vérifier que le fichier existe
    if (access(path.c_str(), F_OK) != 0) {
        setStatus(404);
        setBody("404 Not Found");
        return;
    }
    // Vérifier que je peux modifier le dossier
    if (access(dir.c_str(), W_OK) != 0) {
        setStatus(403);
        setBody("403 Forbidden");
        return;
    }

    // Essayer de supprimer
    if (remove(path.c_str()) != 0) {
        setStatus(500);
        setBody("500 Internal Server Error");
        return;
    }

    // Si suppression réussie → 204 No Content (aucun body)
    setStatus(204);
}
    
void Response::buildFromRequest(const Request& req) {
    if (req.getMethod() == "GET")
        handleGET(req);
    else if (req.getMethod() == "POST")
        handlePOST(req);
    else if (req.getMethod() == "DELETE")
        handleDELETE(req);
    else {
        setStatus(405);
        setBody("Method Not Allowed");
    }
}

// response sent
std::string Response::returnResponse() const {
    std::ostringstream response;

    // 1. Status line
    response << "HTTP/1.1 " << _status << " " << _stateMsg << "\r\n";

    // 2. Headers
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }

    // 3. Ligne vide obligatoire
    response << "\r\n";

    // 4. Body (sauf si code l’interdit)
    if (!(_status == 204 || _status == 304)) {
        response << _body;
    }
    return response.str();
}
