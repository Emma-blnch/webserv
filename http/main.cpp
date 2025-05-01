#include "Request.hpp"

int main(void) 
{
    std::string rawRequest =
    "GET /test.html HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: curl/7.68.0\r\n"
    "Accept: */*\r\n"
    "\r\n";

    Request req;
    req.parseRawRequest(rawRequest);

    std::cout << "Method: " << req.getMethod() << std::endl;
    std::cout << "URI: " << req.getURI() << std::endl;
    std::cout << "Version: " << req.getVersion() << std::endl;
    std::cout << "Host: " << req.getHeader("Host") << std::endl;

    return 0;
}