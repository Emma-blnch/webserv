#include "ServerBlock.hpp"
#include "ServerInstance.hpp"
#include "ConfigUtils.hpp"
#include "LocationBlock.hpp"

ServerInstance createServerInstance(const ServerBlock& block) {
    ServerInstance instance;

    instance.setHost(block.getHost());
    instance.setPort(block.getPort());
    instance.setServerName(block.getServerName());
    instance.setRoot(block.getRoot());
    instance.setIndex(block.getIndex());

    // Convertir clientMaxBodySize en octets
    const std::string& sizeStr = block.getClientMaxBodySize();
    long size = std::atoi(sizeStr.c_str());
    if (!sizeStr.empty() && isdigit(sizeStr[sizeStr.length() - 1]) == 0) {
        char unit = tolower(sizeStr[sizeStr.length() - 1]);
        if (unit == 'k')
            size *= 1024;
        else if (unit == 'm')
            size *= 1024 * 1024;
        else if (unit == 'g')
            size *= 1024 * 1024 * 1024;
    }
    instance.setClientMaxBodySize(static_cast<size_t>(size));

    // Ajouter les pages d'erreur
    const std::map<int, std::string>& errorPages = block.getErrorPages();
    for (std::map<int, std::string>::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it)
        instance.addErrorPage(it->first, it->second);

    // Ajouter les blocs location
    const std::vector<LocationBlock>& locations = block.locations;
    for (size_t i = 0; i < locations.size(); ++i)
        instance.addLocation(locations[i]);

    return instance;
}

bool fillLocationBlock(LocationBlock& loc) {
    for (size_t i = 0; i < loc.directives.size(); ++i) {
        const Directive& dir = loc.directives[i];

        if (dir.key == "allow_methods") {
            loc.allowedMethods = splitLine(dir.value, " \t");
            if (loc.allowedMethods.empty()) {
                std::cout << "Erreur config: allow_methods vide\n";
                return false;
            }
            for (size_t j = 0; j < loc.allowedMethods.size(); ++j) {
                const std::string& method = loc.allowedMethods[j];
                if (method != "GET" && method != "POST" && method != "DELETE") {
                    std::cout << "Erreur config: méthode non supportée : " << method << "\n";
                    return false;
                }
            }
        }
        else if (dir.key == "root") {
            loc.root = dir.value;
            if (access(loc.root.c_str(), F_OK) != 0) {
                std::cout << "Erreur config: root inexistant : " << loc.root << "\n";
                return false;
            }
        }
        else if (dir.key == "index") {
            loc.index = splitLine(dir.value, " \t");
            if (loc.index.empty()) {
                std::cout << "Erreur config: index vide\n";
                return false;
            }
            for (size_t j = 0; j < loc.index.size(); ++j) {
                std::string fullPath = loc.root + "/" + loc.index[j];
                if (access(fullPath.c_str(), F_OK) == 0)
                    break;
                if (j == loc.index.size() - 1) {
                    std::cout << "Erreur config: aucun fichier index trouvé dans " << loc.root << "\n";
                    return false;
                }
            }
        }
        else if (dir.key == "autoindex") {
            loc.autoindex = (dir.value == "on");
        }
        else if (dir.key == "upload_dir") {
            loc.uploadDir = dir.value;
            if (access(loc.uploadDir.c_str(), W_OK) != 0) {
                std::cout << "Erreur config: upload_dir inaccessible ou inexistant : " << loc.uploadDir << "\n";
                return false;
            }
        }
        else if (dir.key == "cgi_path") {
            loc.cgiPath = dir.value;
            if (access(loc.cgiPath.c_str(), X_OK) != 0) {
                std::cout << "Erreur config: cgi_path non exécutable : " << loc.cgiPath << "\n";
                return false;
            }
        }
        else {
            std::cout << "Erreur config: directive inconnue dans location : " << dir.key << "\n";
            return false;
        }
    }
    return true;
}

