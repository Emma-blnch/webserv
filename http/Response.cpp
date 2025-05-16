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
        std::string path = it->second;
        std::ifstream file(path.c_str());
        if (file) {
            std::cout << "page loaded : " << path << std::endl;
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

// POST
bool Response::checkContentLength(const std::string& body, const Request& req) {
    std::string contentLenStr = req.getHeader("Content-Length");
    std::istringstream iss(contentLenStr);
    size_t expectedLength;
    iss >> expectedLength;
    if (iss.fail() || expectedLength != body.size()) {
        setStatus(400);
        setBody("400 Bad Request: Incorrect body size");
        return false;
    }
    return true;
}

bool Response::checkBodySize(const std::string& body, const ServerBlock& server, const LocationBlock* location) {
    size_t maxSize = location ? location->maxBodySize : server.getClientMaxBodySize();
    if (maxSize > 0 && body.size() > maxSize) {
        setStatus(413);
        setBody("413 Payload Too Large");
        return false;
    }
    return true;
}

void Response::handleMultipart(const Request& req, const std::string& body, const std::string& contentType) {
    // Extraire le boundary
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        setStatus(400);
        setBody("400 Bad Request: missing boundary");
        return;
    }
    std::string boundary = "--" + contentType.substr(boundaryPos + 9);
    size_t pos = body.find(boundary);
    if (pos == std::string::npos) {
        setStatus(400);
        setBody("400 Bad Request: no multipart boundary found");
        return;
    }
    // Parser chaque partie
    size_t end;
    while ((end = body.find(boundary, pos + boundary.size())) != std::string::npos) {
        std::string part = body.substr(pos + boundary.size() + 2, end - pos - boundary.size() - 4);
        size_t headerEnd = part.find("\r\n\r\n");
        if (headerEnd == std::string::npos) continue;

        std::string headers = part.substr(0, headerEnd);
        std::string content = part.substr(headerEnd + 4);
        // Extraire filename
        size_t namePos = headers.find("filename=\"");
        if (namePos != std::string::npos) {
            size_t nameEnd = headers.find("\"", namePos + 10);
            std::string filename = headers.substr(namePos + 10, nameEnd - namePos - 10);
            std::ofstream out(("uploads/" + filename).c_str());
            out << content;
            out.close();
        }
        pos = end;
    }
    setStatus(201);
    setBody("File(s) uploaded successfully");
    setHeader("Content-Type", "text/plain");
}

void Response::handleUrlEncoded(const Request& req, const std::string& body) {
    std::map<std::string, std::string> params;
    std::istringstream iss(body);
    std::string pair;
    while (std::getline(iss, pair, '&')) {
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = pair.substr(0, eq);
            std::string value = pair.substr(eq + 1);
            params[key] = value;
        }
    }
    std::ostringstream oss;
    oss << "Received form data:\n";
    for (std::map<std::string, std::string>::iterator it = params.begin(); it != params.end(); ++it)
        oss << it->first << " = " << it->second << "\n";
    setStatus(200);
    setBody(oss.str());
    setHeader("Content-Type", "text/plain");
}

void Response::handlePlainText(const Request& req, const std::string& body) {
    setStatus(200);
    setBody("Received plain text:\n" + body);
    setHeader("Content-Type", "text/plain");
}

void Response::handlePOST(const Request& req, const ServerBlock& server, const LocationBlock* location) { // Envoyer des données au serveur
    // Verifier si methode est allowed selon bloc Location du server
    if (!isMethodAllowed(location, "POST")) {
        setStatus(405);
        return;
    }
    // Vérifier le content-type
    std::string contentType = req.getHeader("content-type");
    // Vérifier la présence d’un body
    std::string body = req.getBody();
    // Check qu'ils sont cohérents
    if (!checkContentLength(body, req)) return;
    if (!checkBodySize(body, server, location)) return;
    // Appel sous-fonctions selon content-type
    if (contentType.find("multipart/form-data") == 0)
        handleMultipart(req, body, contentType);
    else if (contentType.find("application/x-www-form-urlencoded") == 0)
        handleUrlEncoded(req, body);
    else if (contentType.find("text/plain") == 0)
        handlePlainText(req, body);
    else {
        setStatus(415); // Unsupported Media Type
        setBody("Unsupported Content-Type in POST");
    }
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

// CGI
void Response::handleCGI(const Request& req, const LocationBlock* location) {
    // Verifier method allowed dans location block dans config file
    if (!isMethodAllowed(location, req.getMethod())) {
        setStatus(405);
        return;
    }
    // recup interpreteur dans location block
    if (!location || location->cgiPath.empty()) {
        setStatus(500);
        setBody("500 Internal Server Error: CGI script not defined");
        return;
    }
    std::string interpreter = location->cgiPath;
    // protection : seulement python autorisé
    if (interpreter.find("python") == std::string::npos) {
        setStatus(500);
        setBody("500 Internal Server Error: unsupported CGI interpreter");
        return;
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
            (char*)interpreter.c_str(),          // interpréteur
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
        handlePOST(req, server, location);
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
