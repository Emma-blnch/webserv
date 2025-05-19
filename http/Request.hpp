#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <algorithm>

class Request {
    private:
        std::string _method;
        std::string _path;
        std::string _version;
        std::map<std::string, std::string> _header;
        std::string _body;
        std::string _query;

        void parseRequestLine(const std::string& line);
        void parseHeaderLine(const std::string& line);
        void parsePostRequest(const std::string& line);

    public:
        Request() {};
        ~Request() {};

        void parseRawRequest(const std::string& req);

        std::string getMethod() const;
        std::string getPath() const;
        std::string getVersion() const;
        std::string getHeader(const std::string& key) const;
        std::string getBody() const;
        std::string getQuery() const;
};

#endif