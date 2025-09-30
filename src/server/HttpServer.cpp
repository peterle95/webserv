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

/* ORIGINAL IMPLEMENTATION:
  - Blocking single-threaded server using accept(), recv(), sendAll()
  - Each request processed sequentially: accept -> read entire request -> parse -> generate response -> send -> close
  - Could only handle one client at a time
  - Used blocking I/O operations that would wait indefinitely

 NEW IMPLEMENTATION (Non-blocking select()-based event loop):
  - Event-driven architecture using select() system call for I/O multiplexing
  - Non-blocking sockets with fcntl(O_NONBLOCK) to prevent blocking on I/O operations
  - Master fd_sets (read/write) rebuilt each iteration and passed to select()
  - Per-client state management using std::map<int, ClientState>
 */

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
     
     const char* onceEnv = std::getenv("WEBSERV_ONCE");
     bool serveOnce = (onceEnv && std::string(onceEnv) == "1");
     
     printStartupMessage(serveOnce);
     
     int result = runAcceptLoop(server_fd, serveOnce);
     
     close(server_fd);
     DEBUG_PRINT(RED << "Server shutdown complete" << RESET);
     
     return result;
 }