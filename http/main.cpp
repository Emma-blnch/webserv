#include "Request.hpp"

void testRequest(const std::string& rawRequest, const std::string& testName) {
    std::cout << "=== Test: " << testName << " ===" << std::endl;
    try {
        Request req;
        req.parseRawRequest(rawRequest);
        std::cout << "Method: " << req.getMethod() << std::endl;
        std::cout << "Path: " << req.getPath() << std::endl;
        std::cout << "Version: " << req.getVersion() << std::endl;
        std::cout << "Host: " << req.getHeader("Host") << std::endl;
        std::cout << "User-Agent: " << req.getHeader("User-Agent") << std::endl;
        std::cout << "Content-Length: " << req.getHeader("Content-Length") << std::endl;
        std::cout << "Body: " << req.getBody() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    std::cout << std::endl;
}

int main(void) 
{
    // Requête GET valide
    std::string getRequest =
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: curl/7.68.0\r\n"
        "\r\n";

    // Requête POST valide
    std::string postRequest =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: curl/7.68.0\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World";

    // Requête mal formée : ligne de requête incomplète
    std::string badRequest1 =
        "GET HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    // Requête mal formée : méthode inconnue
    std::string badRequest2 =
        "PATCH /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    testRequest(getRequest, "GET valide");
    testRequest(postRequest, "POST valide");
    testRequest(badRequest1, "Requête mal formée 1 (request line)");
    testRequest(badRequest2, "Requête mal formée 2 (méthode non supportée)");
    return 0;
}