#include "http/Request.hpp"
#include "http/Response.hpp"
#include "config/ConfigFile.hpp"
#include "config/LocationBlock.hpp"
#include "config/ServerBlock.hpp"

#include <sys/socket.h> // pour socket, bind, listen, accept, recv, send...
#include <netinet/in.h>     // pour sockaddr_in, htons, htonl, INADDR_ANY...
#include <arpa/inet.h>      // pour inet_addr, inet_ntoa...
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <poll.h>
#include <netinet/in.h>

#define BUFFER_SIZE 8192

int main() {
    // parser fichier de config
    ConfigFile config;
    if (!config.parseConfigFile("file.conf")) {
        std::cerr << "Erreur parsing config.\n";
        return 1;
    }

    std::vector<ServerBlock> servers = config.getServers();
    if (servers.empty()) {
        std::cerr << "Aucun bloc serveur valide.\n";
        return 1;
    }

    const ServerBlock& server = servers[0];

    // créer une socket serveur
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    // configurer son adresse
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(server.getPort());
    std::cout << "port " << server.getPort() << std::endl;
    serverAddress.sin_addr.s_addr = inet_addr(server.getHost().c_str());
    std::cout << "host " << server.getHost() << std::endl;
    // serverAddress.sin_port = htons(8080);
    // serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind le socket à un certain port
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    // écouter les connexions
    listen(serverSocket, 5);

    // préparer poll avec socket serveur
    std::vector<struct pollfd> fds;
    struct pollfd serverFd;
    serverFd.fd = serverSocket;
    serverFd.events = POLLIN; // = "préviens moi quand il ya des connexions"
    fds.push_back(serverFd);

    char buffer[BUFFER_SIZE + 1];

    while (true) {
        int pollCount = poll(&fds[0], fds.size(), -1);
        if (pollCount < 0) {
            std::cerr << "Poll error\n";
            break;
        }

        // check qui a causé l'activité
        for (size_t i = 0; i < fds.size(); ++i) {
            if (fds[i].revents & POLLIN) { // revents contient évènements détectés
                // si c serverSocket alors nouvelle connexion
                if (fds[i].fd == serverSocket) {
                    int clientSocket = accept(serverSocket, NULL, NULL);
                    if (clientSocket >= 0) {
                        struct pollfd clientFd;
                        clientFd.fd = clientSocket;
                        clientFd.events = POLLIN;
                        fds.push_back(clientFd);
                    }
                }
                // si c un client existant je lis sa requête
                else {
                    int clientFd = fds[i].fd;
                    int bytesRead = recv(clientFd, buffer, BUFFER_SIZE, 0);
                    if (bytesRead <= 0) {
                        std::cout << "Client disconnected\n";
                        close(clientFd);
                        fds.erase(fds.begin() + i);
                        --i;
                        continue;
                    }
                    buffer[bytesRead] = '\0';

                    try {
                        Request req;
                        req.parseRawRequest(buffer);
                
                        Response res;
                        res.setErrorPages(server.getErrorPages());
                        res.buildFromRequest(req, server);
                        std::string responseStr = res.returnResponse();
                
                        send(clientFd, responseStr.c_str(), responseStr.size(), 0);
                    } catch (const std::exception& e) {
                        std::cerr << "Erreur : " << e.what() << std::endl;
                
                        // Envoi réponse 400
                        const char* errorResponse =
                            "HTTP/1.1 400 Bad Request\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 11\r\n"
                            "\r\n"
                            "Bad Request";
                        send(clientFd, errorResponse, strlen(errorResponse), 0);
                    }
                    close(clientFd);
                    fds.erase(fds.begin() + i);
                    --i;
                }
            }
        }
    }

    close(serverSocket);
    return 0;
}