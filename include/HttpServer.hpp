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

class HttpServer
{
private:
    int         _port;
    std::string _root;
    std::string _index;

public:
    HttpServer(int port, const std::string &root, const std::string &index);
    ~HttpServer();

    // Start a very simple blocking server for demo purposes
    // Returns 0 on normal exit, non-zero on error
    // TODO: make this non-blocking. Achieve this by using poll() to wait for connections.
    int start();
};

#endif
