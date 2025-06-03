<p align="center">
  <img src="https://github.com/ayogun/42-project-badges/blob/main/badges/webserve.png">
</p>

# 💽 Webserv

A team project from 42 Paris: implement a non-blocking HTTP server in C++98.   

> 🧑‍💻 Developed in collaboration with [Alan Boukezia](https://github.com/aelaen-1) 👥 during our 42 cursus.  

## 🚀 Project goals

- Understand what a web server is and how it works
- Understand and use HTTP methods such as `GET`, `POST` and `DELETE`
- Never block, using a single call to `poll()` (or equivalent)
- The server should never crash or freeze
- Manage edge cases, errors, and complex requests

## 🔧 Features

✅ HTTP Methods: GET, POST, DELETE  
✅ Python script execution in `/cgi-bin/...`, with timeout management (504 Gateway Timeout)   
✅ Non-blocking: A single call to `poll()` iterated in the main loop, handling client connections in `pollfd` structures
✅ Error handling: customizable error pages, exact status codes (400, 403, 404, 405, 413, 414, 500, 501, 504, 505, ...)   
✅ Body size limit: `client_max_body_size` directive, with return 413 if exceeded   
✅ Auto-index: directory listing if no `index` page found (or if `autoindex on`)   
✅ Nginx-inspired configuration: `server { ... }` and `location { ... }` blocks, directives (`listen host:port;`, `server_name foo.com bar.com;`, `root /path/to/www;`...)   

## 🧠 What we learned

- **Working in pairs:** shared version management, permanent communication and joint decision-making on code architecture.  

- **Organization and rigor:** division of the project into modules (config parser, query management, responses, CGI, etc.), compliance with 42 standards (Makefile, Norme, error management).  

- **Solving complex problems:** managing redirects, pipes (`fork`/`execve`/`dup2`), CGI timeout, and handling HTTP/1.1 borderline cases (fragmented requests, non-blocking client and server).  

- **Reading and interpretation of system documentation:** understanding and use of system calls such as `socket`, `bind`, `listen`, `accept`, `poll`, `fork`, `execve`, `waitpid`, `dup2`, `access`, `stat`, etc.  

- **Autonomy and perseverance:** debugging recalcitrant bugs (signal management, timeouts, memory leaks), cross-testing with `curl`, `siege` and `valgrind`, and comparing our behavior with that of Nginx.  

- **Robustness:** write code that is resilient to client errors (ill-formed requests, overly large bodies, buggy CGIs), while ensuring that no crashes or hangs occur, even under heavy load.

## 🧪 Prerequisites

- A C++ compiler compatible with C++98 (e.g. `g++` or `clang++`)   
- GNU build tools such as `make`   
- Unix-like (Linux, macOS)   
- Python 3 (for CGI tests)   
- (Optional) `curl`, `siege`, `valgrind` for testing and profiling   

## 📁 Project structure
```
.
├── cgi-bin/
│   ├── hello.py
│   ├── test.py
│   └── post.py
├── config_file/
│   ├── file.conf
│   └── nginx.conf
├── config/
│   ├── ConfigFile.cpp
│   ├── ConfigFile.hpp
│   ├── ServerBlock.cpp
│   ├── ServerBlock.hpp
│   ├── LocationBlock.cpp
│   └── ParsingUtils.cpp
├── http/
│   ├── handleGet.hpp
│   ├── handleGet.cpp
│   ├── handlePost.hpp
│   ├── handlePost.cpp
│   ├── handleDelete.hpp
│   ├── handleDelete.cpp
│   ├── Response.cpp
│   └── Request.cpp
├── www/
│   ├── fonts/
│   ├── img/
│   ├── style.css
│   └── index.html
├── Makefile
└── README.md
```

## 📚 Resources
- [Webserv - building a non-blocking webserver](https://m4nnb3ll.medium.com/webserv-building-a-non-blocking-web-server-in-c-98-a-42-project-04c7365e4ec7)   
- [HTTP Request - MDN web docs](https://developer.mozilla.org/fr/docs/Web/HTTP/Reference/Methods)   
- - [Webserv doc](https://hackmd.io/@fttranscendance/H1mLWxbr_)   

## 👨‍💻 Authors
Emma Blanchard  
Alan Boukezia