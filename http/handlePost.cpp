#include "handlePost.hpp"

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

void Response::handleMultipart(const std::string& body, const std::string& contentType) {
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

void Response::handleUrlEncoded(const std::string& body) {
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

void Response::handlePlainText(const std::string& body) {
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
        handleMultipart(body, contentType);
    else if (contentType.find("application/x-www-form-urlencoded") == 0)
        handleUrlEncoded(body);
    else if (contentType.find("text/plain") == 0)
        handlePlainText(body);
    else {
        setStatus(415); // Unsupported Media Type
        setBody("Unsupported Content-Type in POST");
    }
    std::cout << "Méthode POST réussie" << std::endl;
}