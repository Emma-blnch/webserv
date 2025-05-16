#include "utils.hpp"
#include "../config/LocationBlock.hpp"
#include <algorithm>

const std::map<int, std::string>& getValidStatus() {
    static std::map<int, std::string> m;
    if (m.empty()) {
        m[200] = "OK";
        m[201] = "Created";
        m[204] = "No Content";
        m[400] = "Bad Request";
        m[403] = "Forbidden";
        m[404] = "Not Found";
        m[405] = "Method Not Allowed";
        m[413] = "Payload Too Large";
        m[414] = "URI Too Long";
        m[500] = "Internal Server Error";
        m[501] = "Not Implemented";
        m[505] = "HTTP Version Not Supported";
    }
    return m;
}

const std::map<std::string, std::string>& getValidMimeTypes() {
    static std::map<std::string, std::string> m;
    if (m.empty()) {
        m[".html"] = "text/html";
        m[".htm"] = "text/html";
        m[".css"] = "text/css";
        m[".js"] = "application/javascript";
        m[".json"] = "application/json";
        m[".txt"] = "text/plain";
        m[".png"] = "image/png";
        m[".jpg"] = "image/jpeg";
        m[".jpeg"] = "image/jpeg";
        m[".gif"] = "image/gif";
        m[".pdf"] = "application/pdf";
        m[".ico"] = "image/x-icon";
    }
    return m;
}

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

std::string normalizePath(const std::string& path) {
    std::vector<std::string> parts;
    std::istringstream iss(path);
    std::string token;

    while (std::getline(iss, token, '/')) {
        if (token.empty() || token == ".")
            continue;
        if (token == "..") {
            if (!parts.empty())
                parts.pop_back(); // monte d'un dossier
        }
        else {
            parts.push_back(token);
        }
    }
    std::string cleanPath = "/";
    for (size_t i = 0; i < parts.size(); ++i) {
        cleanPath += parts[i];
        if (i + 1 != parts.size())
            cleanPath += "/";
    }
    return cleanPath;
}

bool isMethodAllowed(const LocationBlock* location, const std::string& method) {
    if (!location) // pas de bloc location donc aucune restriction ?
        return true;
        
    for (size_t i = 0; i < location->allowedMethods.size(); ++i) {
        if (location->allowedMethods[i] == method) {
            return true;
        }
    }
    return false;
}