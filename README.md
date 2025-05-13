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
