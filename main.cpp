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
    std::vector<const ServerBlock*> serverBlocks;
};

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./webserv [CONFIG FILE].conf" << std::endl;
        return 1;
    }
    ConfigFile config;
    if (!config.parseConfigFile(argv[1])) {
        // std::cerr << "Erreur parsing config.\n";
        return 1;
    }
    std::vector<ServerBlock> servers = config.getServers();
    if (servers.empty()) {
        std::cerr << "Aucun bloc serveur valide.\n";
        return 1;
    }
    std::vector<SocketData> sockets;
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerBlock& server = servers[i];
        std::string host = server.getHost();
        int port = server.getPort();
        bool found = false;
        for (size_t j = 0; j < sockets.size(); ++j) {
            if (sockets[j].host == host && sockets[j].port == port) {
                sockets[j].serverBlocks.push_back(&server);
                found = true;
                break;
            }
        }
        if (!found) {
            int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket < 0) {
                std::cerr << "Socket creation failed for " << host << ":" << port << std::endl;
                return 1;
            }
            int yes = 1;
            if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
                std::cerr << "setsockopt(SO_REUSEADDR) failed\n";
                close(serverSocket);
                continue;
            }
            sockaddr_in serverAddress;
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_port = htons(port);
            serverAddress.sin_addr.s_addr = inet_addr(host.c_str());
            std::cout << "port " << port << std::endl;
            std::cout << "host " << host << std::endl;
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
            SocketData data;
            data.fd = serverSocket;
            data.host = host;
            data.port = port;
            data.serverBlocks.push_back(&server);
            sockets.push_back(data);
        }
    }
    std::vector<struct pollfd> fds;
    for (size_t i = 0; i < sockets.size(); ++i) {
        struct pollfd serverFd;
        serverFd.fd = sockets[i].fd;
        serverFd.events = POLLIN;
        fds.push_back(serverFd);
    }
    std::map<int, size_t> clientSocketToServer;
    std::map<int, std::string> clientBuffers; // Ajout : buffer par client

    char buffer[BUFFER_SIZE + 1];
    const size_t ABSOLUTE_MAX_REQUEST_SIZE = 20 * 1024 * 1024; // Limite "hard" anti-trolls : 20 Mo

    while (true) {
        int pollCount = poll(&fds[0], fds.size(), -1);
        if (pollCount < 0) {
            std::cerr << "Poll error\n";
            break;
        }
        for (size_t i = 0; i < fds.size(); ++i) {
            bool isServerFd = false;
            size_t serverIndex = 0;
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
            if (fds[i].revents & POLLIN) {
                int clientFd = fds[i].fd;
                int bytesRead = recv(clientFd, buffer, BUFFER_SIZE, 0);
                if (bytesRead <= 0) {
                    std::cout << "Client disconnected\n";
                    close(clientFd);
                    fds.erase(fds.begin() + i);
                    clientBuffers.erase(clientFd);
                    clientSocketToServer.erase(clientFd);
                    --i;
                    continue;
                }
                buffer[bytesRead] = '\0';
                clientBuffers[clientFd].append(buffer, bytesRead);
                // Anti flood : hard limit absolue sur la requête
                if (clientBuffers[clientFd].size() > ABSOLUTE_MAX_REQUEST_SIZE) {
                    const char* errorResponse =
                        "HTTP/1.1 413 Payload Too Large\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 22\r\n"
                        "\r\n"
                        "413 Payload Too Large";
                    send(clientFd, errorResponse, strlen(errorResponse), 0);
                    close(clientFd);
                    fds.erase(fds.begin() + i);
                    clientBuffers.erase(clientFd);
                    clientSocketToServer.erase(clientFd);
                    --i;
                    continue;
                }
                try {
                    size_t socketIndex = clientSocketToServer[clientFd];
                    const std::vector<const ServerBlock*>& serverBlocks = sockets[socketIndex].serverBlocks;

                    Request req;
                    req.parseRawRequest(clientBuffers[clientFd]);

                    std::string hostHeader = req.getHeader("host");
                    std::string hostName = hostHeader;
                    size_t colon = hostHeader.find(':');
                    if (colon != std::string::npos)
                        hostName = hostHeader.substr(0, colon);
                    const ServerBlock* chosenServer = NULL;
                    for (size_t s = 0; s < serverBlocks.size(); ++s) {
                        const std::vector<std::string>& names = serverBlocks[s]->getServerNames();
                        for (size_t n = 0; n < names.size(); ++n) {
                            if (hostName == names[n]) {
                                chosenServer = serverBlocks[s];
                                break;
                            }
                        }
                        if (chosenServer)
                            break;
                    }
                    if (!chosenServer)
                        chosenServer = serverBlocks[0];

                    Response res;
                    res.setErrorPages(chosenServer->getErrorPages());
                    res.buildFromRequest(req, *chosenServer);
                    std::string responseStr = res.returnResponse();
                    send(clientFd, responseStr.c_str(), responseStr.size(), 0);

                    close(clientFd);
                    fds.erase(fds.begin() + i);
                    clientBuffers.erase(clientFd);
                    clientSocketToServer.erase(clientFd);
                    --i;
                } catch (const std::runtime_error& e) {
                    std::string errStr = e.what();
                    if (errStr.find("Incomplete request body") != std::string::npos) {
                        continue;
                    }
                    else if (errStr.find("Payload Too Large") != std::string::npos) {
                        const char* errorResponse =
                            "HTTP/1.1 413 Payload Too Large\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 22\r\n"
                            "\r\n"
                            "413 Payload Too Large";
                        send(clientFd, errorResponse, strlen(errorResponse), 0);
                        close(clientFd);
                        fds.erase(fds.begin() + i);
                        clientBuffers.erase(clientFd);
                        clientSocketToServer.erase(clientFd);
                        --i;
                        continue;
                    }
                    else {
                        const char* errorResponse =
                            "HTTP/1.1 400 Bad Request\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 11\r\n"
                            "\r\n"
                            "Bad Request";
                        send(clientFd, errorResponse, strlen(errorResponse), 0);
                        close(clientFd);
                        fds.erase(fds.begin() + i);
                        clientBuffers.erase(clientFd);
                        clientSocketToServer.erase(clientFd);
                        --i;
                        continue;
                    }
                }
            }
        }
    }
    for (size_t i = 0; i < sockets.size(); ++i)
        close(sockets[i].fd);
    return 0;
}
