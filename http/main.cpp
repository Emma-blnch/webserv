#include "Request.hpp"
#include "Response.hpp"
#include "../config/ServerBlock.hpp"
#include "../config/LocationBlock.hpp"

// TESTS REQUEST & REPONSES :

// void testRequestAndResponse(const std::string& rawRequest, const std::string& testName) {
//     std::cout << "=== Test: " << testName << " ===" << std::endl;
//     try {
//         Request req;
//         req.parseRawRequest(rawRequest);

//         std::cout << "[Request parsed successfully]" << std::endl;
//         std::cout << "Method: " << req.getMethod() << std::endl;
//         std::cout << "Path: " << req.getPath() << std::endl;
//         std::cout << "Version: " << req.getVersion() << std::endl;
//         std::cout << "Host: " << req.getHeader("Host") << std::endl;

//         if (!req.getBody().empty()) {
//             std::cout << "Body: " << req.getBody() << std::endl;
//         }

//         Response res;
//         res.buildFromRequest(req);
//         std::string responseText = res.returnResponse();

//         std::cout << "\n--- HTTP Response ---" << std::endl;
//         std::cout << responseText << std::endl;

//     } catch (const std::exception& e) {
//         std::cerr << "[Error] " << e.what() << std::endl;
//     }

//     std::cout << "=============================\n" << std::endl;
// }

// int main(void) {
//     std::string getRequest =
//         "GET /index.html HTTP/1.1\r\n"
//         "Host: localhost\r\n"
//         "User-Agent: curl/7.68.0\r\n"
//         "\r\n";

//     std::string postRequest =
//         "POST /submit HTTP/1.1\r\n"
//         "Host: localhost\r\n"
//         "User-Agent: curl/7.68.0\r\n"
//         "Content-Length: 11\r\n"
//         "\r\n"
//         "Hello World";

//     std::string deleteRequest =
//         "DELETE /file.txt HTTP/1.1\r\n"
//         "Host: localhost\r\n"
//         "\r\n";

//     std::string badRequest1 =
//         "GET HTTP/1.1\r\n"
//         "Host: localhost\r\n"
//         "\r\n";

//     std::string badRequest2 =
//         "PATCH /index.html HTTP/1.1\r\n"
//         "Host: localhost\r\n"
//         "\r\n";

//     std::string invalidPost =
//         "POST /submit HTTP/1.1\r\n"
//         "Host: localhost\r\n"
//         "Content-Length: 20\r\n"
//         "\r\n"
//         "Short";

//     testRequestAndResponse(getRequest, "GET valide");
//     testRequestAndResponse(postRequest, "POST valide");
//     testRequestAndResponse(deleteRequest, "DELETE valide");
//     testRequestAndResponse(badRequest1, "Mauvaise ligne de requête");
//     testRequestAndResponse(badRequest2, "Méthode non supportée");
//     testRequestAndResponse(invalidPost, "POST avec body trop court");

//     return 0;
// }


// TEST REQUETE ET REPONSE AVEC SERVEUR MINIMAL :
#include <sys/socket.h> // pour socket, bind, listen, accept, recv, send...
#include <netinet/in.h>     // pour sockaddr_in, htons, htonl, INADDR_ANY...
#include <arpa/inet.h>      // pour inet_addr, inet_ntoa...
#include <unistd.h>
#include <cstring>
#include <string>

int main() {
    // créer une socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // configurer son adress
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind le socket à un certain port
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    // écouter les connexions
    listen(serverSocket, 5);

    // accepter les clients
    int clientSocket = accept(serverSocket, NULL, NULL);

    // recevoir le message du client
    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer) - 1, 0);  // -1 pour null terminator
    buffer[sizeof(buffer) - 1] = '\0'; // sécurité

    try {
        Request req;
        req.parseRawRequest(buffer);
        // std::cout << "buffer " << buffer;

        Response res;
       
        std::map<int, std::string> errorPages;
        errorPages[404] = "/errors/404.html";
        errorPages[403] = "/errors/403.html";
        errorPages[500] = "/errors/500.html";
        res.setErrorPages(errorPages);
        const ServerBlock server;
        res.buildFromRequest(req, server);
        std::string responseStr = res.returnResponse();

        send(clientSocket, responseStr.c_str(), responseStr.size(), 0);
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << std::endl;

        // Envoi réponse 400
        const char* errorResponse =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "Bad Request";
        send(clientSocket, errorResponse, strlen(errorResponse), 0);
    }

    close(serverSocket);
}

// dans un autre terminal :
// curl -i http://localhost:8080/index.html
// curl -X POST http://localhost:8080 -d "hello"