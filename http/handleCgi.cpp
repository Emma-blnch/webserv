#include "handleCgi.hpp"

std::vector<std::string> Response::buildCGIEnvp(const Request& req) {
    std::vector<std::string> envs;
    envs.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envs.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envs.push_back("REQUEST_METHOD=" + req.getMethod());
    envs.push_back("SCRIPT_NAME=" + req.getPath());
    envs.push_back("QUERY_STRING=" + req.getQuery());

    std::string contentLength = req.getHeader("Content-Length");
    if (!contentLength.empty())
        envs.push_back("CONTENT_LENGTH=" + contentLength);
    std::string contentType = req.getHeader("Content-Type");
    if (!contentType.empty())
        envs.push_back("CONTENT_TYPE=" + contentType);
    return envs;
}

std::string Response::readCGIOutput(int fd) {
    char buffer[1024];
    std::string output;
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    close(fd);
    return output;
}

void Response::handleCGI(const Request& req, const LocationBlock* location, const ServerBlock& server) {
    // Verifier method allowed dans location block dans config file
    if (!isMethodAllowed(location, req.getMethod())) {
        setStatus(405);
        return;
    }
    // Verifier taille du body
    if (req.getMethod() == "POST") {
        if (!checkContentLength(req.getBody(), req))
            return;
        size_t maxSize = location ? location->maxBodySize : server.getClientMaxBodySize();
        if (maxSize > 0 && req.getBody().size() > maxSize) {
            setStatus(413);
            setBody("413 Payload Too Large");
            return;
        }
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
    std::string scriptRel = req.getPath().substr(location->path.length());
    std::string scriptPath = location->root;
    if (scriptPath.empty() || scriptPath[scriptPath.size()-1] != '/')
        scriptPath += "/";
    scriptPath += location->path;
    if (!scriptPath.empty() && scriptPath[scriptPath.size()-1] != '/')
        scriptPath += "/";
    scriptPath += scriptRel;
    // vérifier que le script existe et est exécutable
    if (!checkFilePermissions(scriptPath, F_OK, 404)) return;
    if (!checkFilePermissions(scriptPath, X_OK, 403)) return;
    // créer un pipe pour récupérer la sortie du script
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        setStatus(500);
        return;
    }
    // pipe pour entree (body quand methode POST)
    int inputPipe[2];
    if (req.getMethod() == "POST"  && pipe(inputPipe) == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        setStatus(500);
        return;
    }
    pid_t pid = fork();
    if (pid < 0) {
        setStatus(500);
        return;
    }
    else if (pid == 0) { // CHILD
        // redirige pipe sur STDOUT
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); // ferme lecture
        close(pipefd[1]);
        if (req.getMethod() == "POST") {
            dup2(inputPipe[0], STDIN_FILENO); // redirige pipe sur STDIN
            close(inputPipe[0]);
            close(inputPipe[1]);
        }
        // construire variables d'env
        std::string method = req.getMethod();
        std::string query = req.getQuery();
        std::string contentLength = req.getHeader("Content-Length");
        std::string contentType = req.getHeader("Content-Type");
        std::vector<std::string> envs = buildCGIEnvp(req);
        // conversion en char*[] pour execve
        std::vector<char*> envp;
        for (size_t i = 0; i < envs.size(); ++i)
            envp.push_back(const_cast<char*>(envs[i].c_str ()));
        envp.push_back(NULL);
        // prépare args pour execve
        char* const argv[] = {
            (char*)interpreter.c_str(),          // interpréteur
            (char*)scriptPath.c_str(),          // script à exécuter
            NULL
        };
        execve(interpreter.c_str(), argv, &envp[0]);
        // si execve échoue
        exit(1);
    }
    else { // PARENT
        close(pipefd[1]); // lit la sortie du script
        if (req.getMethod() == "POST") {
            close(inputPipe[0]);
            // ecrit le body dans entree du cgi
            write(inputPipe[1], req.getBody().c_str(), req.getBody().size());
            close(inputPipe[1]);
        }
        std::string output = readCGIOutput(pipefd[0]);
        waitpid(pid, NULL, 0); // attend fin du processus enfant
        // préparer la réponse
        setStatus(200);
        setBody(output);
        setHeader("Content-Type", "text/plain");
    }
}