#include "Response.hpp"
#include "Request.hpp"
#include "utils.hpp"
#include "../config/ServerBlock.hpp"

// pour load nos pages d'erreurs html perso
void Response::loadErrorPageIfNeeded() {
    if (_status == 204 || _status == 304)
        return;

    std::map<int, std::string>::const_iterator it = _errorPages.find(_status);
    if (it != _errorPages.end()) {
        std::string root = "../www";
        std::ifstream file(root + it->second.c_str());
        if (file) {
            std::cout << "page loaded : " << root + it->second << std::endl;
            std::ostringstream content;
            content << file.rdbuf();
            file.close();
            setBody(content.str());
            setHeader("Content-Type", "text/html");
        } else {
            // fallback si le fichier HTML d'erreur n'existe pas
            setBody(getValidStatus().at(_status));
        }
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

// build response for each method :
bool isMethodAllowed(const LocationBlock* location, const std::string& method) {
    if (!location) // pas de bloc location donc aucune restriction ?
        return true;
        
    for (size_t i = 0; i < location->allowedMethods.size(); ++i) {
        if (location->allowedMethods[i] == method) {
            return true;
        }
    }
    return false;
}

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

    // Traduire l'URI en chemin réel
    // std::string path = "." + req.getPath();

    // Vérifier que le fichier existe
    if (access(fullPath.c_str(), F_OK) != 0) {
        setStatus(404);
        // setBody("404 Not Found");
        return;
    }
    // Vérifier que je peux le lire
    if (access(fullPath.c_str(), R_OK) != 0) {
        setStatus(403);
        // setBody("403 Forbidden");
        return;
    }
    // Vérifier que j'arrive à bien lire tout le fichier
    std::ifstream file(fullPath.c_str());
    if (!file) {
        setStatus(500);
        // setBody("500 Internal Server Error");
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

// POST
void Response::handlePOST(const Request& req, const LocationBlock* location) { // Envoyer des données au serveur
    // Verifier si methode est allowed selon bloc Location du server
    if (!isMethodAllowed(location, "POST")) {
        setStatus(405);
        return;
    }
    
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
void Response::handleDELETE(const Request& req, const ServerBlock& server, const LocationBlock* location) { // Supprimer la ressource
    // Verifier si methode est allowed selon bloc Location du server
    if (!isMethodAllowed(location, "DELETE")) {
        setStatus(405);
        return;
    }
    
    // recupere le root du location bloc et construit le chemin avec
    std::string root = location ? location->root : server.getRoot();
    std::string relPath = req.getPath();
    if (location)
        relPath = relPath.substr(location->path.length());

    std::string fullPath = root + relPath;
    std::string dir = fullPath.substr(0, fullPath.find_last_of('/'));

    // Traduire l’URI en chemin réel
    // std::string path = "." + req.getPath();
    // // Extraire le dossier
    // std::string dir = path.substr(0, path.find_last_of('/'));
    if (dir.empty())
        dir = ".";

    // Vérifier que le fichier existe
    if (access(fullPath.c_str(), F_OK) != 0) {
        setStatus(404);
        // setBody("404 Not Found");
        return;
    }
    // Vérifier que je peux modifier le dossier
    if (access(dir.c_str(), W_OK) != 0) {
        setStatus(403);
        // setBody("403 Forbidden");
        return;
    }

    // Essayer de supprimer
    if (remove(fullPath.c_str()) != 0) {
        setStatus(500);
        // setBody("500 Internal Server Error");
        return;
    }

    // Si suppression réussie → 204 No Content (aucun body)
    setStatus(204);
}

// CGI
void Response::handleCGI(const Request& req, const LocationBlock* location) {
    // Verifier method allowed dans location block dans config file
    if (!isMethodAllowed(location, req.getMethod())) {
        setStatus(405);
        return;
    }

    // recup interpreteur dans location block
    std::string interpreter;
    if (location && !location->cgiPath.empty()) {
        interpreter = location->cgiPath;
    }
    else {
        interpreter = "/usr/bin/python3";
    }

    // generer chemin selon location block
    std::string scriptPath;
    if (location && !location->root.empty()) {
        scriptPath = location->root + req.getPath().substr(location->path.length());
    }
    else {
        scriptPath = ".." + req.getPath();
    }

    // vérifier que le script existe et est exécutable
    if (access(scriptPath.c_str(), F_OK) != 0) {
        setStatus(404);
        return;
    }
    if (access(scriptPath.c_str(), X_OK) != 0) {
        setStatus(403);
        return;
    }

    // créer un pipe pour récupérer la sortie du script
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        setStatus(500);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        setStatus(500);
        return;
    }
    else if (pid == 0) { // CHILD
        // redirige STDOUT vers pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); // ferme lecture
        close(pipefd[1]);

        // prépare args pour execve
        char* const argv[] = {
            (char*)"/usr/bin/python3",          // interpréteur
            (char*)scriptPath.c_str(),          // script à exécuter
            NULL
        };

        // prépare variables d'environnement minimales
        char* const envp[] = {
            (char*)"GATEWAY_INTERFACE=CGI/1.1",
            (char*)"SERVER_PROTOCOL=HTTP/1.1",
            (char*)"REQUEST_METHOD=GET",
            NULL
        };
        execve("/usr/bin/python3", argv, envp);
        // si exec échoue
        exit(1);
    }
    else { // PARENT
        close(pipefd[1]); // lit la sortie du script
        char buffer[1024];
        std::string output;

        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        close(pipefd[0]);
        waitpid(pid, NULL, 0); // attend fin du processus enfant

        // préparer la réponse
        setStatus(200);
        setBody(output);
        setHeader("Content-Type", "text/plain");
    }
}

void Response::buildFromRequest(const Request& req, const ServerBlock& server) {
    std::string path = req.getPath();
    const LocationBlock* location = server.findMatchingLocation(req.getPath());

    if (path.find("/cgi-bin/") == 0) {
        handleCGI(req, location);
    }
    else if (req.getMethod() == "GET")
        handleGET(req, server, location);
    else if (req.getMethod() == "POST")
        handlePOST(req, location);
    else if (req.getMethod() == "DELETE")
        handleDELETE(req, server, location);
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
