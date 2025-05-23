# Push du 19/05
- refacto de certaines fonctions dans partie http : découpage en sous fonctions pour plus de lisibilité et modularité
- dans partie config : remplacement des std::cout en std::cerr ou LOG_ERR pour messages d'erreur (hétérogénéité)
- dans partie config : création de 2 mini fonctions pour remplacer std::stol et std::stoi qui n'exsitent qu'a partir de c++11
- dans partie http : handleCGI complétée pour prendre en compte toutes les variables importantes
- main revu pour prendre en compte plusieurs serveurs (au lieu d'un seul avant)

---  

# A faire 

Je crois qu'il n'y a que la directive root qui est obligatoire dans un bloc server. On initialise 
déjà host et port (donc listen pas obligatoire j'imagine).
Aussi, le handleGet empêche la compilation à cause du changement sur index (on peut 
avoir plusieurs index dans la directive, le getIndex renvoie donc un vector de string et pas une string)


--- 

1. [OK] (pour les keys seulement) - Normaliser la casse pour toutes les vérifs de directive   
-> Pour éviter des bugs du type Index ≠ index, tout passer en minuscules (toLower() ou équivalent) avant la comparaison
À faire : partout où compares un dir.key ou la valeur d’une directive (on, off, etc).  


2. [QUASI]Location Blocks (incomplet / partiel)   
on les parse mais on :
- doit pouvoir redéfinir root, allowed_methods, index, cgi_path dans une location.
- doit gérer l’inheritance et la surcharge correcte des directives.
-> Quand un client fait une requête vers un chemin qui match un bloc location, alors s'il y a une directive (root, index, autoindex, allowed_methods, etc.) dans ce bloc location, elle écrase celle du bloc server mais s'il manque une directive dans location, alors le server doit fournir la valeur par défaut.

-- [OK] SAUF POUR CELLES ABSENTES DANS LE BLOC SERVER (allowed_methods, autoindex...), à bien vérifier

-> à checker dans "Response::buildFromRequest()" je pense 


3. [OK] Tester la valeur d’index à la fin, une fois root garanti   
-> déplacer "std::string fullPath = loc.root + "/" + loc.index[j];" à la fin de la fonction pour etre sur que root est défini avant


4. [QUESTION**] - Initialiser toutes les directives avec une valeur par défaut dans le constructeur de LocationBlock   
-> évite les comportements indéfinis   
root = ""
autoindex = false
allowedMethods.clear() ou allowedMethods.push_back("GET")
index.clear()
maxBodySize = 0

** Pourquoi 0 sur maxBodySize ? (on initialise à 1M nos blocs server) Pourquoi clear() et pourquoi mettre la méthode GET 
en particulier si allowed_methods n'est pas précisée ? 

5. [OK] Ajouter un check pour éviter duplication des directives dans location   
un genre de "std::set<std::string> directivesSeen;
for (...) {
    if (!directivesSeen.insert(dir.key).second) {
        std::cerr << "Erreur: directive " << dir.key << " dupliquée dans le bloc location\n";
        return false;
    }
    // suite...
}
"

8. Tests finaux   
Egalement faire :
- Des tests croisés avec curl, telnet, et navigateur.
- Un test de stress : ab -n 1000 -c 100 http://127.0.0.1:8080/index.html
- Check leaks (valgrind)

# -----------------------------------------------
idées de tests :   
   
# 1. Test GET sur /
curl -v http://127.0.0.1:8080/

# 2. Test POST avec un petit body
curl -v -d "toto=data" http://127.0.0.1:8080/upload/

# 3. Test POST trop gros (devrait faire 413)
dd if=/dev/zero bs=12k count=1 | curl -v --data-binary @- http://127.0.0.1:8080/upload/

# 4. Test DELETE sur /delete/ (créer un fichier à la main avant)
touch www/delete/file.txt
curl -X DELETE -v http://127.0.0.1:8080/delete/file.txt

# 5. Test méthode non autorisée
curl -v PUT -v http://127.0.0.1:8080/

# 6. Test sur location avec index absent (devrait renvoyer 403 ou 404)
curl -v http://127.0.0.1:8080/foo/

# 7. Test HTTP malformé (telnet)
telnet 127.0.0.1 8080
GET / HTTP/1.1
Host: test
User-Agent: bidule
(break avec Ctrl+D ou Ctrl+])

