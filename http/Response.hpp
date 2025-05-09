#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include <map>
#include <fstream>
#include <unistd.h>
class Request;

class Response {
    private:
        int _status; // code de retour
        std::string _stateMsg; // "OK", "Not found"...
        std::string _body; // contenu de la réponse
        std::map<std::string, std::string> _headers; // à minima "content-length" et "content-type"
        std::map<int, std::string> _errorPages; // ex: 404 → "./www/errors/404.html"

        void handleGET(const Request& req);
        void handlePOST(const Request& req);
        void handleDELETE(const Request& req);

        std::string getExtension(const std::string& path);
        std::string guessContentType(const std::string& ext);

    public:
        Response() {};
        ~Response() {};

        void setStatus(int code);
        void setHeader(const std::string& key, const std::string& value);
        void setBody(const std::string& body);
        void setErrorPages(const std::map<int, std::string>& pages);

        void buildFromRequest(const Request& req);
        std::string returnResponse() const;

        void loadErrorPageIfNeeded();
};

// const std::map<int, std::string> validStatus = {
//     {200, "OK"},
//     {201, "Created"},
//     {204, "No Content"},
//     {400, "Bad Request"},
//     {403, "Forbidden"},
//     {404, "Not Found"},
//     {405, "Method Not Allowed"},
//     {413, "Payload Too Large"},
//     {414, "URI Too Long"},
//     {500, "Internal Server Error"},
//     {501, "Not Implemented"},
//     {505, "HTTP Version Not Supported"}
// };

// const std::map<std::string, std::string> validMimeTypes = {
//     {".html", "text/html"},
//     {".htm", "text/html"},
//     {".css", "text/css"},
//     {".js", "application/javascript"},
//     {".json", "application/json"},
//     {".txt", "text/plain"},
//     {".png", "image/png"},
//     {".jpg", "image/jpeg"},
//     {".jpeg", "image/jpeg"},
//     {".gif", "image/gif"},
//     {".pdf", "application/pdf"},
//     {".ico", "image/x-icon"},
// };

#endif