#include "handleCgi.hpp"

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