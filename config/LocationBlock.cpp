#include "LocationBlock.hpp"
#include "ServerBlock.hpp"

bool fillLocationBlock(LocationBlock& loc) {
    for (size_t i = 0; i < loc.directives.size(); ++i) {
        const Directive& dir = loc.directives[i];
        if (dir.key == "allow_methods") {
            loc.allowedMethods = splitLine(dir.value, " \t");
            if (loc.allowedMethods.empty()) {
                LOG_ERR("Allow_methods vide");
                return false;
            }
            for (size_t j = 0; j < loc.allowedMethods.size(); ++j) {
                const std::string& method = loc.allowedMethods[j];
                if (method != "GET" && method != "POST" && method != "DELETE") {
                    std::cerr << "Erreur config: méthode non supportée : " << method << "\n";
                    return false;
                }
            }
        }
        else if (dir.key == "root") {
            loc.root = dir.value;
            if (access(loc.root.c_str(), F_OK) != 0) {
                std::cerr << "Erreur config: root inexistant : " << loc.root << "\n";
                return false;
            }
        }
        else if (dir.key == "index") {
            loc.index = splitLine(dir.value, " \t");
            if (loc.index.empty()) {
                LOG_ERR("Index vide");
                return false;
            }
            for (size_t j = 0; j < loc.index.size(); ++j) {
                std::string fullPath = loc.root + "/" + loc.index[j];
                if (access(fullPath.c_str(), F_OK) == 0)
                    break;
                if (j == loc.index.size() - 1) {
                    std::cerr << "Erreur config: aucun fichier index trouvé dans " << loc.root << "\n";
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
                std::cerr << "Erreur config: upload_dir inaccessible ou inexistant : " << loc.uploadDir << "\n";
                return false;
            }
        }
        else if (dir.key == "cgi_path") {
            loc.cgiPath = dir.value;
            if (access(loc.cgiPath.c_str(), X_OK) != 0) {
                std::cerr << "Erreur config: cgi_path non exécutable : " << loc.cgiPath << "\n";
                return false;
            }
        }
        else if (dir.key == "client_max_body_size") {
            std::string sizeStr = dir.value;
            if (sizeStr.empty()) {
                LOG_ERR("Client_max_body_size vide dans location");
                return false;
            }

            long size = std::atoi(sizeStr.c_str());
            if (!sizeStr.empty() && isdigit(sizeStr[sizeStr.length() - 1]) == 0) {
                char unit = tolower(sizeStr[sizeStr.length() - 1]);
                if (unit == 'k') size *= 1024;
                else if (unit == 'm') size *= 1024 * 1024;
                else if (unit == 'g') size *= 1024 * 1024 * 1024;
            }

            if (size <= 0) {
                LOG_ERR("Taille invalide pour client_max_body_size dans location");
                return false;
            }

            loc.maxBodySize = static_cast<size_t>(size);
        }
        else {
            std::cerr << "Erreur config: directive inconnue dans location : " << dir.key << "\n";
            return false;
        }
    }
    return true;
}