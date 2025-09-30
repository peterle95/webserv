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

HttpServer::HttpServer(int port, const std::string &root, const std::string &index)
    : _port(port), _root(root), _index(index) {}

HttpServer::~HttpServer() {}

int HttpServer::start()
 {
     int server_fd = createAndBindSocket();
     if (server_fd < 0)
         return 1;
 
     if (!setNonBlocking(server_fd))
         return 1;
     
    // Use environment variable to control serve-once mode
    // This allows running the server in a single-request mode for testing
     const char* onceEnv = std::getenv("WEBSERV_ONCE");
     bool serveOnce = (onceEnv && std::string(onceEnv) == "1");
     
     printStartupMessage(serveOnce);
     
     int result = runAcceptLoop(server_fd, serveOnce);
     
     close(server_fd);
     DEBUG_PRINT(RED << "Server shutdown complete" << RESET);
     
     return result;
 }