/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 14:20:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/10/24 00:00:00 by pmolzer          ###   .fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"

HttpServer::HttpServer(ConfigParser &configParser) : _configParser(configParser)
{
    _servers = configParser.getServers();
    _root = configParser.getRoot();
    _index = configParser.getIndex();
    mapCurrentLocationConfig("/", 0); // default location
}

HttpServer::~HttpServer() {}

// Map the current location config based on the request path
void HttpServer::mapCurrentLocationConfig(const std::string &path, const int serverIndex)
{
    const std::map<std::string, LocationConfig> &locations = _servers[serverIndex].getLocations();

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
std::string HttpServer::getFilePath(const std::string &path, const int serverIndex)
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
                filePath = _currentLocation->root + path + _servers[serverIndex].getIndex(); // fallback to server index
            }
        }
        else
        {
            if (!_currentLocation->index.empty())
                filePath = _servers[serverIndex].getRoot() + path + _currentLocation->index;
            else
                filePath = _servers[serverIndex].getRoot() + path + _servers[serverIndex].getIndex(); // fallback to server index
        }
    }
    else
    {
        (!_currentLocation->root.empty()) ? filePath = _currentLocation->root + path
                                          : filePath = _servers[serverIndex].getRoot() + path;
    }

    return filePath;
}

// Public helper to resolve file path for a request path and map location
std::string HttpServer::resolveFilePathFor(const std::string &path, const int serverIndex)
{
    mapCurrentLocationConfig(path, serverIndex);
    return getFilePath(path, serverIndex);
}

// for use in response.cpp return the current location config
const LocationConfig *HttpServer::getCurrentLocation()
{
    return _currentLocation;
}

int HttpServer::start()
{
    // Iterate through all server blocks
    for (size_t serverIdx = 0; serverIdx < _servers.size(); ++serverIdx)
    {
        const ServerConfig &serverConfig = _servers[serverIdx];
        const std::vector<std::pair<std::string, int> > &listenAddresses = serverConfig.getListenAddresses();

        DEBUG_PRINT("Setting up server block " << serverIdx
                                               << " (" << serverConfig.getServerName() << ") with "
                                               << listenAddresses.size() << " listen addresses");

        // Create socket for each host:port pair in this server block
        for (size_t addrIdx = 0; addrIdx < listenAddresses.size(); ++addrIdx)
        {
            const std::string &host = listenAddresses[addrIdx].first;
            int port = listenAddresses[addrIdx].second;

            int server_fd = createAndBindSocket(host, port);
            if (server_fd < 0)
            {
                std::cerr << "Failed to bind server " << serverIdx
                          << " to " << host << ":" << port << std::endl;
                continue; // Try other addresses
            }

            if (!setNonBlocking(server_fd))
            {
                close(server_fd);
                std::cerr << "Failed to set non-blocking for server " << serverIdx
                          << " " << host << ":" << port << std::endl;
                continue;
            }

            _serverSockets.push_back(ServerSocketInfo(server_fd, host, port, serverIdx));
            std::cout << "Server block " << serverIdx
                      << " (" << serverConfig.getServerName()
                      << ") listening on " << host << ":" << port << std::endl;
        }
    }

    if (_serverSockets.empty())
    {
        std::cerr << "No sockets could be created for any server block" << std::endl;
        return 1;
    }
    // Use environment variable to control serve-once mode
    // This allows running the server in a single-request mode for testing
    const char *onceEnv = std::getenv("WEBSERV_ONCE");
    bool serveOnce = (onceEnv && std::string(onceEnv) == "1");

    printStartupMessage(serveOnce);

    // Call the aligned accept loop
    int result = runMultiServerAcceptLoop(_serverSockets, serveOnce);

    // Cleanup
    for (size_t i = 0; i < _serverSockets.size(); ++i)
    {
        close(_serverSockets[i].socket_fd);
    }
    return result;
}
// Case-insensitive string equality for ASCII
static inline bool iequals(const std::string &a, const std::string &b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        unsigned char ca = static_cast<unsigned char>(a[i]);
        unsigned char cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb))
            return false;
    }
    return true;
}

size_t HttpServer::selectServerForRequest(const HTTPparser &parser, const int serverPort)
{
    std::string host;
    std::string portStr;

    host = parser.getServerName();
    portStr = parser.getServerPort();

    // Iterate through server configs to find a matching server name and port
    for (size_t i = 0; i < _servers.size(); ++i)
    {
        const ServerConfig &serverConfig = _servers[i];
        const std::string &serverName = serverConfig.getServerName();
        const std::vector<int> &listenPorts = serverConfig.getListenPorts();

        // Check if server name matches
        if (iequals(serverName, host))
        {
            // Check if the server listens on the requested port
            for (size_t j = 0; j < listenPorts.size(); ++j)
            {
                if (listenPorts[j] == serverPort)
                {
                    return i; // Found matching server
                }
            }
        }
    }

    // No matching server found, default to first server block
    return static_cast<size_t>(-1);
}
