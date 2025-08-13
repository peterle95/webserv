Here is a repository tree for your HTTP server project, drawing on the information about building a server from scratch and the Webserv project requirements:
websrv/
├── conf/
│   ├── default.conf
│   └── example.conf
├── html/
│   ├── index.html
│   ├── info.html
│   ├── 404.html
│   └── uploads/
├── src/
│   ├── main.cpp
│   ├── Server.cpp
│   ├── Client.cpp
│   ├── HttpRequest.cpp
│   ├── HttpResponse.cpp
│   ├── ConfigParser.cpp
│   ├── FileManager.cpp
│   ├── CgiHandler.cpp
│   └── Utils.cpp
├── include/
│   ├── common.hpp
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── HttpRequest.hpp
│   ├── HttpResponse.hpp
│   ├── ConfigParser.hpp
│   ├── FileManager.hpp
│   ├── CgiHandler.hpp
│   └── Utils.hpp
├── tests/
│   ├── test_client.py
│   ├── stress_test.py
│   └── README.md
├── Makefile
└── README.md
Repository Tree Explanation:
• websrv/: This is the root directory of your project.
• conf/: This directory holds configuration files for your web server.
    ◦ default.conf: An example of a default server configuration file. It should define basic settings like listening ports, default error pages, and accepted HTTP methods for routes.
    ◦ example.conf: Another example configuration file to demonstrate various features, such as multiple interface:port pairs, maximum client request body size, URL/route rules (including HTTP redirection, root directories, directory listing, default files for directories, and file upload locations).
• html/ (or www/): This directory contains the static web content to be served by your HTTP server.
    ◦ index.html: The default page to serve when a client visits the root directory or a folder without a specified file.
    ◦ info.html: An example HTML file that can be requested by a client.
    ◦ 404.html: A custom error page for "Not Found" (404) errors. You should have default error pages if none are provided in the configuration.
    ◦ uploads/: A directory designated for storing files uploaded by clients, as the server must support file uploads.
• src/: This directory contains the source code files (.cpp) for your HTTP server, written in C++98.
    ◦ main.cpp: The entry point of your server application. It will parse the command-line arguments (including the configuration file), initialize the server components, and start the main server loop.
    ◦ Server.cpp: Implements the core server logic, including setting up the listening sockets using socket(), bind(), and listen(). It manages non-blocking I/O operations for all clients using a single poll() (or select(), epoll(), kqueue()) call to monitor both reading and writing simultaneously.
    ◦ Client.cpp: Manages individual client connections. It handles accepting new connections via accept() and orchestrates the reading of requests and sending of responses.
    ◦ HttpRequest.cpp: Responsible for parsing incoming HTTP requests from clients. This includes extracting the request-line (method, URI, HTTP version), request headers (e.g., Host, User-Agent, Content-Length, Content-Type), and the message body. It should also handle unchunking requests if necessary for CGI.
    ◦ HttpResponse.cpp: Responsible for constructing HTTP responses to be sent back to clients. This involves setting the status-line (HTTP version, status code, reason phrase), response headers (e.g., Content-Type, Content-Length), and the response body.
    ◦ ConfigParser.cpp: Parses the server configuration file (.conf files) to extract settings such as port numbers, error page paths, allowed methods, redirection rules, and client body size limits.
    ◦ FileManager.cpp: Handles file system operations, such as reading static web pages (read()), handling directory listings, and managing file uploads (write()).
    ◦ CgiHandler.cpp: Manages the execution of CGI scripts (e.g., PHP, Python). This module will use fork(), execve(), pipe(), waitpid(), and related system calls to communicate with external programs. It ensures the correct environment variables are available to CGI scripts and handles input/output between the server and CGI process.
    ◦ Utils.cpp: Contains general utility functions that can be used across different parts of the server, such as error reporting (strerror, gai_strerror), network byte order conversions (htons, htonl, ntohs, ntohl), and other helper functions.
• include/: This directory contains the header files (.hpp) for the corresponding .cpp files in src/.
    ◦ common.hpp: A header file that includes common standard library headers and defines shared constants or macros used throughout the project.
    ◦ Server.hpp: Declares the Server class and its member functions.
    ◦ Client.hpp: Declares the Client class.
    ◦ HttpRequest.hpp: Declares the HttpRequest class.
    ◦ HttpResponse.hpp: Declares the HttpResponse class.
    ◦ ConfigParser.hpp: Declares the ConfigParser class.
    ◦ FileManager.hpp: Declares the FileManager class.
    ◦ CgiHandler.hpp: Declares the CgiHandler class.
    ◦ Utils.hpp: Declares utility functions.
• tests/: This directory holds testing scripts for verifying server functionality and robustness.
    ◦ test_client.py: A Python script (or other language) to act as a basic HTTP client to send requests and verify responses from your server, simulating web browser behavior.
    ◦ stress_test.py: A script to stress test the server, ensuring it remains available and does not crash under heavy load or client disconnections.
    ◦ README.md: Provides instructions on how to run the tests and interpret their results.
• Makefile: This file defines the build process for your project, including rules for compiling ($(NAME), all), cleaning (clean, fclean), and recompiling (re). It must compile with c++ and the flags -Wall, -Wextra, -Werror, and -std=c++98.
• README.md: A project overview document, explaining what the server does, how to compile and run it, how to use the configuration files, and any important design decisions or limitations.