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
class HttpServer
{
private:
    int         _port;
    std::string _root;
    std::string _index;
    ConfigParser _configParser;
    const LocationConfig *_currentLocation;

    void mapCurrentLocationConfig(const std::string &path);
    std::string getFilePath(const std::string &path);

    // Socket setup
    int createAndBindSocket();
    void setupSignalHandlers();
    void printStartupMessage(bool serveOnce);

    // Accept loop for incoming connections
    int runAcceptLoop(int server_fd, bool serveOnce);

    // Minimal helpers reused by the simplified handler
    bool determineKeepAlive(const HTTPparser& parser);
    std::string generateBadRequestResponse(bool keepAlive);
    std::string generateGetResponse(const std::string& path, bool keepAlive);
    std::string generateMethodNotAllowedResponse(bool keepAlive);
public:
    HttpServer(ConfigParser &configParser);
    ~HttpServer();

    // Simplified per-connection handling (blocking on the accepted socket)
    // NOTE: This is a temporary implementation to keep the server functional
    // without maintaining per-client state. Implement Client class to handle
    // non-blocking I/O and persistent connections.
    void handleClient(int client_fd);

    static bool setNonBlocking(int fd);

    // Start a very simple blocking server for demo purposes
    // Returns 0 on normal exit, non-zero on error
    // TODO: make this non-blocking. Achieve this by using poll() to wait for connections.
    int start();
};

#endif
