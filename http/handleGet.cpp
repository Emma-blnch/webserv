#include "handleGet.hpp"

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

void Response::handleGET(const Request& req, const ServerBlock& server, const LocationBlock* location) {
    // Verifier si methode est allowed selon bloc Location du server
    if (!isMethodAllowed(location, "GET")) {
        setStatus(405);
        return;
    }
    // recupere le root du location bloc et construit le chemin avec
    std::string root = location ? location->root : server.getRoot();
    std::string relPath = req.getPath();
    if (location)
        relPath = relPath.substr(location->path.length());
    std::string fullPath = root + relPath;
    if (access(fullPath.c_str(), F_OK) != 0) {
        setStatus(404);
        return;
    }
    // Vérifier que je peux le lire
    if (access(fullPath.c_str(), R_OK) != 0) {
        setStatus(403);
        return;
    }
    // Vérifier que j'arrive à bien lire tout le fichier
    std::ifstream file(fullPath.c_str());
    if (!file) {
        setStatus(500);
        return;
    }
    // Sinon, lire tout le contenu pour le mettre dans le body
    std::ostringstream content;
    content << file.rdbuf();
    file.close();
    // Déterminer le type MIME (Content-Type)
    std::string ext = getExtension(fullPath);   // -> ".html"
    std::string mime = guessContentType(ext); // -> "text/html"
    // Préparer la réponse
    setStatus(200);
    setBody(content.str());
    setHeader("Content-Type", mime);
}