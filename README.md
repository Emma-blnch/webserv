# Mes ajouts faits
- ajout de pages html custom (page d'accueil + pages d'erreurs)
- ajout d'un dossier cgi-bin contenant un script python executer par les cgi

---  

# A faire apres parsing du fichier de config fini
- chaque bloc `server` doit etre un serveur donc :
- - Créer une structure ou classe ServerInstance contenant host, port, root, index, error_pages, client_max_body_size, ses locations (avec leurs directives)
- - Boucler sur les ServerBlock du parsing, en créant un objet ServerInstance par bloc
- - Vérifier que deux serveurs n’essaient pas d’ouvrir le même couple host:port
- associer chaque host:port à un socket_fd (Pour chaque couple unique, ouvrir un socket(), faire le bind() et listen() dessus, Ajouter le fd de ce socket à poll() pour surveiller les connexions)
- lorsqu’un client se connecte déterminer quel "server" lui répond (ex: Chercher dans ServerInstance celui qui correspond à l’IP:port utilisé et à la directive server_name)
- lors du buildFromRequest(), en plus de la config server, chercher la location la plus spécifique qui matche le path de la requête et appliquer les directives de cette location en priorité sur celles du server
- class Response (http) a modifier : class buildFromRequest doit maintenant recevoir une référence vers le bloc serveur (ServerBlock) correspondant, remplacer chemin du root du server (std::string path = "." + req.getPath(); into std::string path = serverRoot + req.getPath();
où 'serverRoot' est extrait des directives du bloc serveur.), dans handlePOST ajouter une vérification de la taille du corps (max_body_size), gerer les errorPages selon fichier de config

---

## ex de class ServerInstance :
```
#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP

#include <string>
#include <vector>
#include <map>

struct Location {
    std::string path; // "/upload", "/images"...
    std::string root; // racine spécifique à cette location
    std::string index; // ex: "index.html"
    bool autoindex;
    std::vector<std::string> allowedMethods; // GET, POST, DELETE
    std::string cgiPath; // chemin vers script exécutable ou interpréteur
    std::string uploadDir; // destination des fichiers uploadés

    Location() : autoindex(false) {}
};

class ServerInstance {
private:
    std::string _host;              // "127.0.0.1"
    int _port;                      // 8080
    std::string _serverName;        // "localhost", "mydomain.com"
    std::string _root;              // "./www"
    std::string _index;             // "index.html"
    size_t _clientMaxBodySize;      // en octets
    std::map<int, std::string> _errorPages; // 404 → "/errors/404.html"

    std::vector<Location> _locations;

public:
    // Constructors
    ServerInstance() : _port(80), _clientMaxBodySize(1000000) {}

    // Getters
    const std::string& getHost() const { return _host; }
    int getPort() const { return _port; }
    const std::string& getServerName() const { return _serverName; }
    const std::string& getRoot() const { return _root; }
    const std::string& getIndex() const { return _index; }
    size_t getClientMaxBodySize() const { return _clientMaxBodySize; }
    const std::map<int, std::string>& getErrorPages() const { return _errorPages; }
    const std::vector<Location>& getLocations() const { return _locations; }

    // Setters
    void setHost(const std::string& host) { _host = host; }
    void setPort(int port) { _port = port; }
    void setServerName(const std::string& name) { _serverName = name; }
    void setRoot(const std::string& root) { _root = root; }
    void setIndex(const std::string& index) { _index = index; }
    void setClientMaxBodySize(size_t size) { _clientMaxBodySize = size; }
    void addErrorPage(int code, const std::string& path) { _errorPages[code] = path; }
    void addLocation(const Location& loc) { _locations.push_back(loc); }

    // Trouver la location correspondant à un path donné
    const Location* findMatchingLocation(const std::string& requestPath) const {
        for (size_t i = 0; i < _locations.size(); ++i) {
            if (requestPath.find(_locations[i].path) == 0)
                return &_locations[i];
        }
        return NULL;
    }
};
```
