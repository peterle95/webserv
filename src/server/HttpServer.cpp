/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 14:20:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/09/12 15:33:51 by pmolzer          ###   .fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"

HttpServer::HttpServer(ConfigParser &configParser) : _configParser(configParser)
{
    _port = configParser.getListenPort();
    _root = configParser.getRoot();
    _index = configParser.getIndex();
    mapCurrentLocationConfig("/"); // default location
}

HttpServer::~HttpServer() {}

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

    // Install simple signal handlers to allow graceful shutdown in CI
    std::signal(SIGINT, handle_stop_signal);
    std::signal(SIGTERM, handle_stop_signal);

    // Single-request mode for CI when WEBSERV_ONCE=1
    const char* onceEnv = std::getenv("WEBSERV_ONCE");
    bool serveOnce = (onceEnv && std::string(onceEnv) == "1");
    size_t servedCount = 0;

    // we need infinite loop to keep the server running
    while(true)
    {
        struct sockaddr_in cli; // client address that is connecting to the server
        socklen_t clilen = sizeof(cli); 
        int cfd = accept(server_fd, (struct sockaddr*)&cli, &clilen); // accept the connection
        if (cfd < 0)
        {
            if (errno == EINTR && g_stop)
                break; // interrupted by signal, time to stop
            continue;
        }

        char buf[4096]; // buffer to store the request
        std::memset(buf, 0, sizeof(buf));
        ssize_t n = recv(cfd, buf, sizeof(buf)-1, 0);
        if (n <= 0)
        {
            close(cfd);
            continue;
        }

        // Use HTTPparser to process the raw request
        std::string rawRequest(buf, (size_t)n);
        HTTPparser parser;
        parser.parseRequest(rawRequest);

        std::string path = parser.getPath();
        mapCurrentLocationConfig(path);
        std::string filePath = getFilePath(path); // get the file path based on the request path and current location config

        DEBUG_PRINT("Request path: '" << path << "', mapped to file: '" << filePath << "'");
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

        // In CI single-request mode, exit after first served request
        if (serveOnce)
        {
            ++servedCount;
            if (servedCount >= 1)
                break;
        }
    }

    close(server_fd);
    return 0;
}

// Map the current location config based on the request path
void HttpServer::mapCurrentLocationConfig(const std::string &path)
{
    const std::map<std::string, LocationConfig> &locations = _configParser.getLocations();

    // Exact match lookup
    std::map<std::string, LocationConfig>::const_iterator it = locations.find(path);
    if (it != locations.end())
    {
        _currentLocation = &it->second;
    }
}

// Get the full file path based on the request path and
//    current location config
std::string HttpServer::getFilePath(const std::string &path)
{
    std::string filePath;
    if (_currentLocation->path == path)
    {
        if (!_currentLocation->root.empty())
        {
            if (!_currentLocation->index.empty())
            {
                filePath = _currentLocation->root + path + _currentLocation->index;
            }
            else
            {
                filePath = _currentLocation->root + path + _index; // fallback to server index
            }
        }
        else
        {
            if (!_currentLocation->index.empty())
                filePath = _root + path + _currentLocation->index;
            else
                filePath = _root + path + _index; // fallback to server index
        }
    }
    else
    {
        (!_currentLocation->root.empty()) ? filePath = _currentLocation->root + path
                                          : filePath = _root + path;
    }

    return filePath;
}