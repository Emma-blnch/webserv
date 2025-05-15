# Check a faire sur parsing
- variables static bool ne leak pas (sinon remplacer par un std::set
- dans checkServerBlock() : sur le currentDir.key == "listen", le "_port = std::stoi(currentDir.value);" ne marchera pas s'il y a un IP + Port (listen 127.0.0.0:80) mais que s'il y a le port (listen 80)

---  

# A faire 
- - Vérifier que deux serveurs n’essaient pas d’ouvrir le même couple host:port
- associer chaque host:port à un socket_fd (Pour chaque couple unique, ouvrir un socket(), faire le bind() et listen() dessus, Ajouter le fd de ce socket à poll() pour surveiller les connexions)
- lorsqu’un client se connecte déterminer quel "server" lui répond (ex: Chercher dans ServerInstance celui qui correspond à l’IP:port utilisé et à la directive server_name)
- class Response (http) a modifier : dans handlePOST ajouter une vérification de la taille du corps (max_body_size)
