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

struct SocketData {
    int fd;
    std::string host;
    int port;
    std::vector<const ServerBlock*> serverBlocks; // Tous les ServerBlocks pour ce host:port
};

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./webserv [CONFIG FILE].conf" << std::endl;
        return 1;
    }
    // parser fichier de config
    ConfigFile config;
    if (!config.parseConfigFile(argv[1])) {
        std::cerr << "Erreur parsing config.\n";
        return 1;
    }
    // recuperer tous les bloc server
    std::vector<ServerBlock> servers = config.getServers();
    if (servers.empty()) {
        std::cerr << "Aucun bloc serveur valide.\n";
        return 1;
    }
    // stocker tous les socket ouverts
    std::vector<SocketData> sockets;
    // boucle pour recup tous les host port de chaque server
    for (size_t i = 0; i < servers.size(); ++i) {
        // server actuel
        const ServerBlock& server = servers[i];
        std::string host = server.getHost();
        int port = server.getPort();
        // creation socker server pour ce server
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Socket creation failed for " << host << ":" << port << std::endl;
            return 1;
        }
        // configurer son adresse
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        std::cout << "port " << server.getPort() << std::endl;
        serverAddress.sin_addr.s_addr = inet_addr(host.c_str());
        std::cout << "host " << server.getHost() << std::endl;
        // bind et listen
        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            std::cerr << "Bind failed for " << host << ":" << port << std::endl;
            close(serverSocket);
            continue;
        }
        if (listen(serverSocket, 5) < 0) {
            std::cerr << "Listen failed for " << host << ":" << port << std::endl;
            close(serverSocket);
            continue;
        }
        // check si deja socket ouverte pour ce host:port
        bool found = false;
        for (size_t j = 0; j < sockets.size(); ++j) {
            if (sockets[j].host == host && sockets[j].port == port) {
                sockets[j].serverBlocks.push_back(&server);
                found = true;
                break;
            }
        }
        if (!found) {
            SocketData data;
            data.fd = serverSocket;
            data.host = host;
            data.port = port;
            data.serverBlocks.push_back(&server);
            sockets.push_back(data);
        }
    }
    // préparer poll avec socket serveur
    std::vector<struct pollfd> fds;
    // chaque fd dans fds est un socket server avec host:port
    for (size_t i = 0; i < sockets.size(); ++i) {
        struct pollfd serverFd;
        serverFd.fd = sockets[i].fd;
        serverFd.events = POLLIN; // = "préviens moi quand il ya des connexions"
        fds.push_back(serverFd);
    }

    // faire la correspondance clientSocket -> index socket serveur utiliser
    std::map<int, size_t> clientSocketToServer;
    
    char buffer[BUFFER_SIZE + 1];

    while (true) {
        int pollCount = poll(&fds[0], fds.size(), -1);
        if (pollCount < 0) {
            std::cerr << "Poll error\n";
            break;
        }

        // check qui a causé l'activité
        for (size_t i = 0; i < fds.size(); ++i) {
            bool isServerFd = false;
            size_t serverIndex = 0;
            // si c serverSocket alors nouvelle connexion
            for (; serverIndex < sockets.size(); ++serverIndex) {
                if (fds[i].fd == sockets[serverIndex].fd && (fds[i].revents & POLLIN)) {
                    isServerFd = true;
                    break;
                }
            }
            if (isServerFd) {
                int clientSocket = accept(fds[i].fd, NULL, NULL);
                if (clientSocket >= 0) {
                    struct pollfd clientFd;
                    clientFd.fd = clientSocket;
                    clientFd.events = POLLIN;
                    fds.push_back(clientFd);
                    clientSocketToServer[clientSocket] = serverIndex;
                }
                continue;
            }
            // si c un client existant je lis sa requête
            if (fds[i].revents & POLLIN) {
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
                    // trouver a quel socket correspond client
                    size_t socketIndex = clientSocketToServer[clientFd];
                    const std::vector<const ServerBlock*>& serverBlocks = sockets[socketIndex].serverBlocks;
                    // parse request http
                    Request req;
                    req.parseRawRequest(buffer);
                    // choix du bon serverBlock via Host header
                    std::string hostHeader = req.getHeader("host");
                    const ServerBlock* chosenServer = NULL;
                    for (size_t s = 0; s < serverBlocks.size(); ++s) {
                        if (serverBlocks[s]->getServerName() == hostHeader) {
                            chosenServer = serverBlocks[s];
                            break;
                        }
                    }
                    if (!chosenServer)
                        chosenServer = serverBlocks[0];

                    Response res;
                    res.setErrorPages(chosenServer->getErrorPages());
                    res.buildFromRequest(req, *chosenServer);
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
                clientSocketToServer.erase(clientFd);
                --i;
            }
        }
    }
    for (size_t i = 0; i < sockets.size(); ++i)
        close(sockets[i].fd);
    return 0;
}

// un seul server:

// int main(int argc, char **argv) {
//     if (argc < 2) {
//         std::cerr << "Usage: ./webserv [CONFIG FILE].conf" << std::endl;
//         return 1;
//     }
//     // parser fichier de config
//     ConfigFile config;
//     if (!config.parseConfigFile(argv[1])) {
//         std::cerr << "Erreur parsing config.\n";
//         return 1;
//     }

//     std::vector<ServerBlock> servers = config.getServers();
//     if (servers.empty()) {
//         std::cerr << "Aucun bloc serveur valide.\n";
//         return 1;
//     }

//     const ServerBlock& server = servers[0];

//     // créer une socket serveur
//     int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (serverSocket < 0) {
//         std::cerr << "Socket creation failed\n";
//         return 1;
//     }

//     // configurer son adresse
//     sockaddr_in serverAddress;
//     serverAddress.sin_family = AF_INET;
//     serverAddress.sin_port = htons(server.getPort());
//     std::cout << "port " << server.getPort() << std::endl;
//     serverAddress.sin_addr.s_addr = inet_addr(server.getHost().c_str());
//     std::cout << "host " << server.getHost() << std::endl;
//     // serverAddress.sin_port = htons(8080);
//     // serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

//     // bind le socket à un certain port
//     bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

//     // écouter les connexions
//     listen(serverSocket, 5);

//     // préparer poll avec socket serveur
//     std::vector<struct pollfd> fds;
//     struct pollfd serverFd;
//     serverFd.fd = serverSocket;
//     serverFd.events = POLLIN; // = "préviens moi quand il ya des connexions"
//     fds.push_back(serverFd);

//     char buffer[BUFFER_SIZE + 1];

//     while (true) {
//         int pollCount = poll(&fds[0], fds.size(), -1);
//         if (pollCount < 0) {
//             std::cerr << "Poll error\n";
//             break;
//         }

//         // check qui a causé l'activité
//         for (size_t i = 0; i < fds.size(); ++i) {
//             if (fds[i].revents & POLLIN) { // revents contient évènements détectés
//                 // si c serverSocket alors nouvelle connexion
//                 if (fds[i].fd == serverSocket) {
//                     int clientSocket = accept(serverSocket, NULL, NULL);
//                     if (clientSocket >= 0) {
//                         struct pollfd clientFd;
//                         clientFd.fd = clientSocket;
//                         clientFd.events = POLLIN;
//                         fds.push_back(clientFd);
//                     }
//                 }
//                 // si c un client existant je lis sa requête
//                 else {
//                     int clientFd = fds[i].fd;
//                     int bytesRead = recv(clientFd, buffer, BUFFER_SIZE, 0);
//                     if (bytesRead <= 0) {
//                         std::cout << "Client disconnected\n";
//                         close(clientFd);
//                         fds.erase(fds.begin() + i);
//                         --i;
//                         continue;
//                     }
//                     buffer[bytesRead] = '\0';

//                     try {
//                         Request req;
//                         req.parseRawRequest(buffer);
                
//                         Response res;
//                         res.setErrorPages(server.getErrorPages());
//                         res.buildFromRequest(req, server);
//                         std::string responseStr = res.returnResponse();
                
//                         send(clientFd, responseStr.c_str(), responseStr.size(), 0);
//                     } catch (const std::exception& e) {
//                         std::cerr << "Erreur : " << e.what() << std::endl;
                
//                         // Envoi réponse 400
//                         const char* errorResponse =
//                             "HTTP/1.1 400 Bad Request\r\n"
//                             "Content-Type: text/plain\r\n"
//                             "Content-Length: 11\r\n"
//                             "\r\n"
//                             "Bad Request";
//                         send(clientFd, errorResponse, strlen(errorResponse), 0);
//                     }
//                     close(clientFd);
//                     fds.erase(fds.begin() + i);
//                     --i;
//                 }
//             }
//         }
//     }

//     close(serverSocket);
//     return 0;
// }