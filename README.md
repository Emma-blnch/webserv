# Mes ajouts faits
- ajout de pages html custom (page d'accueil + pages d'erreurs)
- ajout d'un dossier cgi-bin contenant un script python executer par les cgi
- parsing fichiers config finit (ajout des directives du bloc locaiton stockees en attribut, pages d'erreur stockees)

---  

# A faire 
- - Vérifier que deux serveurs n’essaient pas d’ouvrir le même couple host:port
- associer chaque host:port à un socket_fd (Pour chaque couple unique, ouvrir un socket(), faire le bind() et listen() dessus, Ajouter le fd de ce socket à poll() pour surveiller les connexions)
- lorsqu’un client se connecte déterminer quel "server" lui répond (ex: Chercher dans ServerInstance celui qui correspond à l’IP:port utilisé et à la directive server_name)
- class Response (http) a modifier : dans handlePOST ajouter une vérification de la taille du corps (max_body_size)
