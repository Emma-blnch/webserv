# Push du 19/05
- refacto de certaines fonctions dans partie http : découpage en sous fonctions pour plus de lisibilité et modularité
- dans partie config : remplacement des std::cout en std::cerr ou LOG_ERR pour messages d'erreur (hétérogénéité)
- dans partie config : création de 2 mini fonctions pour remplacer std::stol et std::stoi qui n'exsitent qu'a partir de c++11
- dans partie http : handleCGI complétée pour prendre en compte toutes les variables importantes
- main revu pour prendre en compte plusieurs serveurs (au lieu d'un seul avant)

---  

# A faire 
1. Normaliser la casse pour toutes les vérifs de directive   
-> Pour éviter des bugs du type Index ≠ index, tout passer en minuscules (toLower() ou équivalent) avant la comparaison
À faire : partout où compares un dir.key ou la valeur d’une directive (on, off, etc).  

2. Location Blocks (incomplet / partiel)   
on les parse mais on :
- doit pouvoir redéfinir root, allowed_methods, index, cgi_path dans une location.
- doit gérer l’inheritance et la surcharge correcte des directives.
-> Quand un client fait une requête vers un chemin qui match un bloc location, alors s'il y a une directive (root, index, autoindex, allowed_methods, etc.) dans ce bloc location, elle écrase celle du bloc server mais s'il manque une directive dans location, alors le server doit fournir la valeur par défaut.
-> à checker dans "Response::buildFromRequest()" je pense

3. Tester la valeur d’index à la fin, une fois root garanti   
-> déplacer "std::string fullPath = loc.root + "/" + loc.index[j];" à la fin de la fonction pour etre sur que root est défini avant

4. Initialiser toutes les directives avec une valeur par défaut dans le constructeur de LocationBlock   
-> évite les comportements indéfinis   
root = ""
autoindex = false
allowedMethods.clear() ou allowedMethods.push_back("GET")
index.clear()
maxBodySize = 0

5. Ajouter un check pour éviter duplication des directives dans location   
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
