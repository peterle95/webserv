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

public:
    HttpServer(ConfigParser &configParser);
    ~HttpServer();

    // Start a very simple blocking server for demo purposes
    // Returns 0 on normal exit, non-zero on error
    // TODO: make this non-blocking. Achieve this by using poll() to wait for connections.
    int start();
};

#endif
