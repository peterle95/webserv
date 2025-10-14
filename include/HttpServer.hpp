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
private:
    int _port;
    std::string _root;
    std::string _index;
    ConfigParser _configParser;
    const LocationConfig *_currentLocation;

    // Active clients keyed by socket fd
    std::map<int, Client*> _clients;

    void mapCurrentLocationConfig(const std::string &path);
    

    // Socket setup
    int createAndBindSocket();
    void setupSignalHandlers();
    void printStartupMessage(bool serveOnce);

    // Accept loop for incoming connections
    int runAcceptLoop(int server_fd, bool serveOnce);

    // Minimal helpers reused by the simplified handler
    
    std::string generateBadRequestResponse(bool keepAlive);
    std::string generateGetResponse(const std::string &path, bool keepAlive);
    std::string generateMethodNotAllowedResponse(bool keepAlive);
    std::string generatePostResponse(const std::string &body, bool keepAlive);
    bool isMethodAllowed(const std::string &method);
    
    size_t checkContentLength(const std::string &request, size_t header_end);
    
public:
    HttpServer(ConfigParser &configParser);
    //HttpServer();//As a default constructor for HTTPResponse
    ~HttpServer();

    std::string getFilePath(const std::string &path);//changed from private for response.cpp
    bool determineKeepAlive(const HTTPparser &parser);//changed from private to public for access in response.cpp
    const LocationConfig *getCurrentLocation();
    std::string processCGI(HTTPparser &parser);//changed from private to public for access in response.cpp

    // Helper: map location for path and return resolved file path
    std::string resolveFilePathFor(const std::string &path);
    // Simplified per-connection handling (blocking on the accepted socket)
    // NOTE: This is a temporary implementation to keep the server functional
    // without maintaining per-client state. Implement Client class to handle
    // non-blocking I/O and persistent connections.
    void handleClient(int client_fd);

    static bool setNonBlocking(int fd);

    int getPort() const { return _port; }

    // Start a very simple blocking server for demo purposes
    // Returns 0 on normal exit, non-zero on error
    // TODO: make this non-blocking. Achieve this by using poll() to wait for connections.
    int start();

    // Public thin wrappers to reuse existing response builders from other modules
    std::string buildBadRequestResponse(bool keepAlive) { return generateBadRequestResponse(keepAlive); }
    std::string buildGetResponse(const std::string &path, bool keepAlive) { return generateGetResponse(path, keepAlive); }
    std::string buildMethodNotAllowedResponse(bool keepAlive) { return generateMethodNotAllowedResponse(keepAlive); }
    std::string buildPostResponse(const std::string &body, bool keepAlive) { return generatePostResponse(body, keepAlive); }
    bool isMethodAllowedPublic(const std::string &method) { return isMethodAllowed(method); }
};

#endif
