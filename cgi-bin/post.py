#!/usr/bin/env python3
import sys, os

# Pour récupérer la taille du body envoyé
length = int(os.environ.get("CONTENT_LENGTH", 0))
post_body = sys.stdin.read(length) if length > 0 else ""

# On écrit dans un fichier
with open("cgi-bin/data.txt", "a") as f:
    f.write(post_body + "\n")

print("Content-Type: text/html\n")
print("<html><body><h1>Data received!</h1></body></html>")
