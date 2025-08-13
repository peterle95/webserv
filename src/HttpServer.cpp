/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 14:20:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/13 15:05:11 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/HttpServer.hpp"
#include "../include/Common.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <sstream>

HttpServer::HttpServer(int port, const std::string &root, const std::string &index)
: _port(port), _root(root), _index(index) {}

HttpServer::~HttpServer() {}

// read file to string, return empty string on error. Uses binary mode to preserve file contents.
static std::string readFileToString(const std::string &path)
{
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
    if (!ifs.good()) 
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return std::string();
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

// send all data to socket, continue until all data is sent or error occurs
static void sendAll(int fd, const char *data, size_t len)
{
    size_t sent = 0;
    while (sent < len)
    {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n <= 0) break;
        sent += (size_t)n;
    }
}

// start the server, return 0 on success, non-zero on error. Blocks until server is stopped.
// This is a very simple blocking server for demo purposes.
// TODO: make this non-blocking. Achieve this by using poll() to wait for connections.
int HttpServer::start()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "socket() failed" << std::endl;
        return 1;
    }

    // this is to allow the server to be restarted quickly without waiting for the port to be released
    // it works by allowing the socket to be bound to an address that is already in use
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // this struct is defined in <netinet/in.h>
    struct sockaddr_in addr;
    // this block of code is to initialize the struct to 0, because it is not initialized by default
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; // AF_INET is the address family for IPv4
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY is a special address that means "any address"
    addr.sin_port = htons((uint16_t)_port); // htons() converts the port number to network byte order

    // bind the socket to the address, we need to bind the socket to the address because 
    // we want to listen on a specific port
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "bind() failed on port " << _port << std::endl;
        close(server_fd);
        return 1;
    }

    // listen for connections, we need to listen for connections because we want to accept connections
    if (listen(server_fd, 10) < 0)
    {
        std::cerr << "listen() failed" << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "Serving " << _root << "/" << _index << " on http://localhost:" << _port << "/" << std::endl;

    // we need infinite loop to keep the server running
    while(true)
    {
        struct sockaddr_in cli; // client address that is connecting to the server
        socklen_t clilen = sizeof(cli); 
        int cfd = accept(server_fd, (struct sockaddr*)&cli, &clilen); // accept the connection
        if (cfd < 0)
            continue;

        char buf[4096]; // buffer to store the request
        std::memset(buf, 0, sizeof(buf));
        ssize_t n = recv(cfd, buf, sizeof(buf)-1, 0);
        if (n <= 0)
        {
            close(cfd);
            continue;
        }

        // Very basic request parsing (GET only)
        std::string req(buf, (size_t)n);
        std::string path = "/";
        if (req.compare(0, 4, "GET ") == 0)
        {
            std::string::size_type sp = req.find(' ');
            if (sp != std::string::npos)
            {
                std::string::size_type sp2 = req.find(' ', sp + 1);
                if (sp2 != std::string::npos)
                    path = req.substr(sp + 1, sp2 - (sp + 1));
            }
        }

        std::string filePath;
        if (path == "/" || path.empty())
            filePath = _root + "/" + _index;
        else
        {
            if (path.size() > 0 && path[0] == '/')
                filePath = _root + path; // maps "/foo" -> "root/foo"
            else
                filePath = _root + "/" + path;
        }

        std::string body = readFileToString(filePath);
        std::ostringstream resp;
        if (!body.empty())
        {
            resp << "HTTP/1.1 200 OK\r\n";
            resp << "Content-Type: text/html; charset=utf-8\r\n";
            resp << "Content-Length: " << body.size() << "\r\n";
            resp << "Connection: close\r\n\r\n";
            resp << body;
        }
        else
        {
            std::string notFound = "<h1>404 Not Found</h1>";
            resp << "HTTP/1.1 404 Not Found\r\n";
            resp << "Content-Type: text/html; charset=utf-8\r\n";
            resp << "Content-Length: " << notFound.size() << "\r\n";
            resp << "Connection: close\r\n\r\n";
            resp << notFound;
        }

        std::string respStr = resp.str();
        sendAll(cfd, respStr.c_str(), respStr.size());
        close(cfd);
    }

    close(server_fd);
    return 0;
}
