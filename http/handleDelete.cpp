#include "handleDelete.hpp"

bool Response::checkFilePermissions(const std::string& file, int mode, int errorCode) {
    if (access(file.c_str(), mode) != 0) {
        setStatus(errorCode);
        return false;
    }
    return true;
}

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
    if (location)
    {
        std::string fullPath = root + location->path + relPath;
        std::string dir = fullPath.substr(0, fullPath.find_last_of('/'));
        if (dir.empty())
            dir = ".";
        struct stat s;
        if (stat(fullPath.c_str(), &s) != 0) {
            setStatus(404);
            return;
        }
        // Vérifier que ce n’est pas un dossier
        if (S_ISDIR(s.st_mode)) {
            setStatus(403);
            return;
        }
        // Vérifier que je peux modifier le dossier parent
        if (access(dir.c_str(), W_OK) != 0) {
            setStatus(403);
            return;
        }
        // Vérifier que je peux modifier (supprimer) le fichier
        if (access(fullPath.c_str(), W_OK) != 0) {
            setStatus(403);
            return;
        }
        // Essayer de supprimer
        if (remove(fullPath.c_str()) != 0) {
            setStatus(500);
            return;
        }
        setStatus(204);
    }
    else
    {
        std::string fullPath = root + relPath;
        std::string dir = fullPath.substr(0, fullPath.find_last_of('/'));
        if (dir.empty())
            dir = ".";
        struct stat s;
        if (stat(fullPath.c_str(), &s) != 0) {
            setStatus(404);
            return;
        }
        // Vérifier que ce n’est pas un dossier
        if (S_ISDIR(s.st_mode)) {
            setStatus(403);
            return;
        }
        // Vérifier que je peux modifier le dossier parent
        if (access(dir.c_str(), W_OK) != 0) {
            setStatus(403);
            return;
        }
        // Vérifier que je peux modifier (supprimer) le fichier
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
    std::cout << "Méthode DELETE réussie" << std::endl;
}
