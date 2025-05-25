# Autres modifs faites
- dans fillLocationBlock modif de cette condition "if (hasIndex || !loc.index.empty())" en "(hasIndex)" car condition TOUJOURS vraie (-> hérite de l'index du server dans tous les cas donc il check l'index server dans le dossier location donc pas bon)   
- petit ajout dans le main pour ne plus avoir de bind fail quand on relance trop vite sur le meme hote port   
- ajout d'une fonction dans ServerBlock.hpp pour mieux gérer le path réel du chemin renseigné dans un bloc location   
- correction de handleGET pour gérer les requêtes sur fichier dans des blocs location   

---   
# A faire 
1. dans void Request::parseRequestLine(const std::string& line)   
renvoyer erreur 405 au lieu de 400 quand mauvaise méthode   

2. [OK] location "./" ne fonctionne pas   
-> dans le fichier de config il faut mettre "location /" mais "root ./" pour que ça fonctionne sinon code était bon   

3. Refacto fonctions pour lisibilité   
-> découper en sous fonctions   
-> s'assurer que chaque fichier a son hpp   
-> s'assurer que les fonctions sont déclarées dans .hpp mais écrites dans .cpp   

4. [OK] Initialiser dans le constructeur de LocationBlock    
allowedMethods.push_back("GET") et POST   

5. [OK?] Revoir page accueil HTML pour pouvoir tout tester dessus   
-> méthodes GET POST et DELETE fonctionnent   
-> est-ce qu'on rajoute autre chose ?   

6. [OK] Rajouter message log quand une méthode a réussit   
-> ok pour GET POST et DELETE

7. [OK] Correction de handlePOST pour pouvoir upload un fichier et le récupérer

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

