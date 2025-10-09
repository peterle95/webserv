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

// Map the current location config based on the request path
void HttpServer::mapCurrentLocationConfig(const std::string &path)
{
    const std::map<std::string, LocationConfig> &locations = _configParser.getLocations();

    size_t longestMatchLength = 0;
    const LocationConfig *bestLocation = NULL;

    // Find the longest matching location prefix
    for (std::map<std::string, LocationConfig>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        const std::string &locationPath = it->first;

        // Check if the request path starts with this location path
        if (path.size() >= locationPath.size() &&
            path.substr(0, locationPath.size()) == locationPath)
        {
            // If this is a longer match than what we've found so far, use it
            if (locationPath.size() > longestMatchLength)
            {
                longestMatchLength = locationPath.size();
                bestLocation = &it->second;
            }
        }
    }

    // Set the best matching location, or keep current if no match found
    if (bestLocation != NULL)
    {
        _currentLocation = bestLocation;
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