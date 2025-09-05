/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 14:20:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/09/05 15:07:39 by pmolzer          ###   ########.fr       */
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
    // TODO: Pass host parameter (currently hardcoded to "127.0.0.1")
    return socket_server("127.0.0.1", _port);
}