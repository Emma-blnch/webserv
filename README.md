# Check a faire sur parsing
- variables static bool ne leak pas (sinon remplacer par un std::set

---  

# A faire 
1. Routing / Matching des ServerBlocks   
Actuellement on utilise servers[0] pour répondre -> implémenter la gestion des différents serveurs
- Identifier le bon ServerBlock à partir du couple (host, port) + Host: header.

2. Location Blocks (incomplet / partiel)   
on les parse mais on :
- doit pouvoir redéfinir root, allowed_methods, index, cgi_path dans une location.
- doit faire le matching le plus précis (longest match path).
- doit gérer l’inheritance et la surcharge correcte des directives.
-> Quand un client fait une requête vers un chemin qui match un bloc location, alors s'il y a une directive (root, index, autoindex, allowed_methods, etc.) dans ce bloc location, elle écrase celle du bloc server mais s'il manque une directive dans location, alors le server doit fournir la valeur par défaut.
-> à checker dans "Response::buildFromRequest()" je pense

3. Tests finaux   
Egalement faire :
- Des tests croisés avec curl, telnet, et navigateur.
- Un test de stress : ab -n 1000 -c 100 http://127.0.0.1:8080/index.html
- Check leaks (valgrind)
