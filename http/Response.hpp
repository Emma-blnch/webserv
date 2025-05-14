#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include <map>
#include <fstream>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>

class ServerInstance;
class Request;

class Response {
    private:
        int _status; // code de retour
        std::string _stateMsg; // "OK", "Not found"...
        std::string _body; // contenu de la réponse
        std::map<std::string, std::string> _headers; // à minima "content-length" et "content-type"
        std::map<int, std::string> _errorPages; // ex: 404 → "./www/errors/404.html"

        void handleGET(const Request& req, const ServerInstance& server, const LocationBlock* location);
        void handlePOST(const Request& req, const ServerInstance& server, const LocationBlock* location);
        void handleDELETE(const Request& req, const ServerInstance& server, const LocationBlock* location);

        std::string getExtension(const std::string& path);
        std::string guessContentType(const std::string& ext);

    public:
        Response() {};
        ~Response() {};

        void setStatus(int code);
        void setHeader(const std::string& key, const std::string& value);
        void setBody(const std::string& body);
        void setErrorPages(const std::map<int, std::string>& pages);

        void buildFromRequest(const Request& req, const ServerInstance& server);
        // void buildFromRequest(const Request& req);
        std::string returnResponse() const;

        void loadErrorPageIfNeeded();

        void handleCGI(const Request& req, const ServerInstance& server, const LocationBlock* location);
};

#endif