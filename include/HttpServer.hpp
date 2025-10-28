/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 14:20:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/13 15:15:00 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include "ConfigParser.hpp"

#include "HTTPparser.hpp"
#include <map>

class Client; // forward declaration

class HttpServer
{
    // Structure to track socket info
    struct ServerSocketInfo
    {
        int socket_fd;
        int port;
        size_t serverIndex; // Which server block this belongs to

        ServerSocketInfo(int fd, int p, size_t idx)
            : socket_fd(fd), port(p), serverIndex(idx) {}
    };

private:
    // int _port;
    std::string _root;
    std::string _index;
    ConfigParser _configParser;
    const LocationConfig *_currentLocation;
    std::vector<ServerConfig> _servers; // Store multiple server configs
    // Active clients keyed by socket fd
    std::map<int, Client *> _clients;
    std::vector<ServerSocketInfo> _serverSockets;

    void mapCurrentLocationConfig(const std::string &path, const int serverIndex);

    // Socket setup
    int createAndBindSocket(int port, in_addr_t host);
    void setupSignalHandlers();
    void printStartupMessage();

    // Accept loop for incoming connections
    //int runAcceptLoop(int server_fd);
    int runMultiServerAcceptLoop(const std::vector<ServerSocketInfo> &serverSockets);

public:
    HttpServer(ConfigParser &configParser);
    // HttpServer();//As a default constructor for HTTPResponse
    ~HttpServer();

    std::string getFilePath(const std::string &path, const int serverIndex); // changed from private for response.cpp
    bool determineKeepAlive(const HTTPparser &parser);                       // changed from private to public for access in response.cpp
    const LocationConfig *getCurrentLocation();
    std::string processCGI(HTTPparser &parser); // changed from private to public for access in response.cpp

    // Helper: map location for path and return resolved file path
    std::string resolveFilePathFor(const std::string &path, const int serverIndex);

    static bool setNonBlocking(int fd);

    // int getPort() const { return _port; }

    // Start the non-blocking HTTP server with Client class state machine
    // Returns 0 on normal exit, non-zero on error
    int start();

    // Response generation methods (public for Client access)
    std::string generateBadRequestResponse(bool keepAlive);
    std::string generateGetResponse(const std::string &path, bool keepAlive);
    std::string generateMethodNotAllowedResponse(bool keepAlive);
    std::string generatePostResponse(const std::string &body, bool keepAlive);
    bool isMethodAllowed(const std::string &method);
    size_t selectServerForRequest(const HTTPparser &parser, const int serverPort);
};

#endif
