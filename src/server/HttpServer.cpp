/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 14:20:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/09/05 10:44:46 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Common.hpp"

HttpServer::HttpServer(int port, const std::string &root, const std::string &index)
: _port(port), _root(root), _index(index) {}

HttpServer::~HttpServer() {}

int HttpServer::start()
{
    std::cout << "Starting server on port " << _port << std::endl;
    std::cout << "Serving content from: " << _root << std::endl;
    std::cout << "Default index file: " << _index << std::endl;
    
    // Call the polling-based server function from server.cpp
    // TODO: Pass host parameter (currently hardcoded to "127.0.0.1" in server.cpp)
    return socket_server("127.0.0.1", _port);
}

// The following functions are removed since server.cpp handles the core server logic:
// - readFileToString() - moved to server.cpp or separate utility
// - sendAll() - moved to server.cpp or separate utility  
// - Signal handling - already in server.cpp
// - Socket setup and main loop - handled by socket_server()

// TODO: Next steps to complete the integration:
// 1. Modify server.cpp to use the _root and _index values for file serving
// 2. Add HTTP request parsing and response generation in server.cpp
// 3. Move file reading utilities from HttpServer.cpp to server.cpp
// 4. Ensure server.cpp uses the configuration values (root, index) passed from HttpServer
// 5. Add proper error handling and logging integration
// 6. Consider making the host configurable instead of hardcoded "127.0.0.1"
// 7. Integrate the signal handling and graceful shutdown from the original HttpServer