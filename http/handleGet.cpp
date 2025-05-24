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

std::string generateAutoIndex(const std::string& dirPath, const std::string& urlPath) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir)
        return "<html><body><h1>Unable to open directory</h1></body></html>";
    // commence a ecrire html
    std::ostringstream html;
    html << "<html><head><title>Index of " << urlPath << "</title></head><body>";
    html << "<h1>Index of " << urlPath << "</h1><ul>";
    // lit le prochain element du dossier ouvert (fichier ou sous dossier)
    struct dirent* entry;
    // boucle s'arrete quand tous fichiers lus
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        // on saute le dossier courant
        if (name == ".")
            continue;
        // liste html avec un lien vers chaque fichier/sous dossier
        html << "<li><a href=\"" << urlPath;
        if (urlPath[urlPath.length()-1] != '/')
            html << "/";
        html << name << "\">" << name << "</a></li>";
    }
    // ferme le dossier
    closedir(dir);
    html << "</ul></body></html>";
    // retourne chaine html complete
    return html.str();
}

void Response::handleGetDirectory(const std::string& dirPath, const std::string& urlPath, const ServerBlock& server, const LocationBlock* location) {
    std::vector<std::string> indexList;
    // si index dans bloc location
    // for (size_t i = 0; i < location->index.size(); ++i){
    //     std::cout << location->index[i] << std::endl;
    // }
    if (location && !location->index.empty())
        indexList = location->index;
    // sinon je prends index du bloc server
    else if (!server.getIndex().empty())
    {
        indexList.insert(indexList.end(), server.getIndex().begin(), server.getIndex().end());
        // for (size_t i = 0; i < indexList.size(); ++i){
        //     std::cout << indexList[i] << std::endl;
        // }
    }
    // sinon je met mon fichier de base
    else
        indexList.push_back("index.html");
    // parcours la liste des fichiers d'index possibles pour les tester
    for (size_t i = 0; i < indexList.size(); ++i) {
        std::string candidate = dirPath;
        if (candidate[candidate.size() - 1] != '/')
            candidate += "/";
        candidate += indexList[i];
        // si le fichier existe
        if (access(candidate.c_str(), F_OK) == 0) {
            // je l'ouvre dans un ifstream
            std::ifstream file(candidate.c_str());
            if (!file) {
                setStatus(500);
                return;
            }
            // on lit tout le contenu et on le met dans le body
            std::ostringstream content;
            content << file.rdbuf();
            file.close();
            std::string ext = getExtension(candidate);
            std::string mime = guessContentType(ext);
            setStatus(200);
            setBody(content.str());
            setHeader("Content-Type", mime);
            return;
        }
    }
    // si pas index et autoindex activer
    if (location && location->autoindex) {
        // genere HTML d'autoindex
        std::string html = generateAutoIndex(dirPath, urlPath);
        setStatus(200);
        setBody(html);
        setHeader("Content-Type", "text/html");
        return;
    }
    // si pas index et autoindex desactiver
    setStatus(403);
    setBody("403 Forbidden: Directory listing denied");
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
    // Si c'est un dossier :
    struct stat s;
    // S_ISDIR(s.st_mode) = macro qui teste si le chemin pointer est dossier
    if (stat(fullPath.c_str(), &s) == 0 && S_ISDIR(s.st_mode)) {
        handleGetDirectory(fullPath, req.getPath(), server, location);
        return;
    }
    // Si pas dossier, vérifier que j'arrive à bien lire tout le fichier
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
    // LOG
    std::cout << "Méthode GET réussie" << std::endl;
}