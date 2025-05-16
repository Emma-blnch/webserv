#include "handleDelete.hpp"

void Response::handleDELETE(const Request& req, const ServerBlock& server, const LocationBlock* location) { // Supprimer la ressource
    // Verifier si methode est allowed selon bloc Location du server
    if (!isMethodAllowed(location, "DELETE")) {
        setStatus(405);
        return;
    }
    // recupere le root du location bloc et construit le chemin avec
    std::string root = location ? location->root : server.getRoot();
    std::string relPath = req.getPath();
    if (location && req.getPath().find(location->path) == 0)
        relPath = relPath.substr(location->path.length());
    else
        relPath = req.getPath();
    std::string fullPath = root + relPath;
    std::string dir = fullPath.substr(0, fullPath.find_last_of('/'));
    if (dir.empty())
        dir = ".";
    // Vérifier que ce que je veux supprimer n'est pas un dossier
    struct stat s;
    if (stat(fullPath.c_str(), &s) == 0 && S_ISDIR(s.st_mode)) {
        setStatus(403);
        return;
    }
    // Vérifier que le fichier existe
    if (access(fullPath.c_str(), F_OK) != 0) {
        setStatus(404);
        return;
    }
    // Vérifier que je peux modifier le dossier
    if (access(dir.c_str(), W_OK) != 0) {
        setStatus(403);
        return;
    }
    // Vérifier que je peux modifier le fichier
    if (access(fullPath.c_str(), W_OK) != 0) {
        setStatus(403);
        return;
    }
    // Essayer de supprimer
    if (remove(fullPath.c_str()) != 0) {
        setStatus(500);
        return;
    }
    // Si suppression réussie → 204 No Content (aucun body)
    setStatus(204);
}