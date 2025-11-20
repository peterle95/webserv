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
        bool matches = (path.size() >= locationPath.size() &&
                        path.substr(0, locationPath.size()) == locationPath);

        // If this is a longer match than what we've found so far, use it
        if (matches && locationPath.size() > longestMatchLength)
        {
            longestMatchLength = locationPath.size();
            bestLocation = &it->second;
        }
        // Set the best matching location, or keep current if no match found
        /*if (locationPath.size() > longestMatchLength )
        {
            _currentLocation = bestLocation;
        }*/
    }
    _currentLocation = bestLocation;
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
            else if (_currentLocation->autoindex)
                filePath = _currentLocation->root + path;
            else
            {
                filePath = _currentLocation->root + path + _servers[serverIndex].getIndex(); // fallback to server index
            }
        }
        else
        {
            if (!_currentLocation->index.empty())
                filePath = _servers[serverIndex].getRoot() + path + _currentLocation->index;
            else if (_currentLocation->autoindex)
                filePath = _servers[serverIndex].getRoot() + path;
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
    // Pre-validation: Check all configurations before attempting to start any servers
    if (!validateConfiguration())
    {
        std::cerr << "Validation of Config File failed. Server will not start." << std::endl;
        return 1;
    }

    // Iterate through all server blocks
    for (size_t serverIdx = 0; serverIdx < _servers.size(); ++serverIdx)
    {
        const ServerConfig &serverConfig = _servers[serverIdx];
        const std::vector<int> &ports = serverConfig.getListenPorts();

        DEBUG_PRINT("Setting up server block " << serverIdx
                                               << " (" << serverConfig.getServerName() << ") with "
                                               << ports.size() << " ports");

        // Create socket for each port in this server block
        for (size_t portIdx = 0; portIdx < ports.size(); ++portIdx)
        {
            int port = ports[portIdx];

            // Temporarily set _port for createAndBindSocket() to use
            //_port = port;

            // Pass the host address to the socket creation function.
            int server_fd = createAndBindSocket(port, serverConfig.getHost());
            if (server_fd < 0)
            {
                std::cerr << "Failed to bind server " << serverIdx
                          << " to port " << port << std::endl;
                continue; // Try other ports
            }

            if (!setNonBlocking(server_fd))
            {
                close(server_fd);
                std::cerr << "Failed to set non-blocking for server " << serverIdx
                          << " port " << port << std::endl;
                continue;
            }

            _serverSockets.push_back(ServerSocketInfo(server_fd, port, serverIdx));
            // Convert the host address back to a string for logging.
            char hostStr[INET_ADDRSTRLEN];
            struct in_addr host_addr;
            host_addr.s_addr = serverConfig.getHost();
            inet_ntop(AF_INET, &host_addr, hostStr, INET_ADDRSTRLEN);
            std::cout << "Server block " << serverIdx
                      << " (" << serverConfig.getServerName()
                      << ") listening on " << hostStr << ":" << port << std::endl;
        }
    }

    if (_serverSockets.empty())
    {
        std::cerr << "No sockets could be created for any server block" << std::endl;
        return 1;
    }
    printStartupMessage();

    // Call the aligned accept loop
    int result = runMultiServerAcceptLoop(_serverSockets);

    // Cleanup
    for (size_t i = 0; i < _serverSockets.size(); ++i)
    {
        close(_serverSockets[i].socket_fd);
    }
    return result;
}

// Pre-validation to check all configurations before server startup
bool HttpServer::validateConfiguration()
{
    std::cout << "Running pre-validation on " << _servers.size() << " server blocks..." << std::endl;

    bool allValid = true;
    std::map<int, size_t> portToServerIndex;

    for (size_t serverIdx = 0; serverIdx < _servers.size(); ++serverIdx)
    {
        const ServerConfig &serverConfig = _servers[serverIdx];
        const std::vector<int> &ports = serverConfig.getListenPorts();

        std::cout << "Validating server block " << serverIdx + 1
                  << " (" << serverConfig.getServerName() << ")..." << std::endl;

        // First Check: Port conflicts
        // TODO: Will need to be revisited if multiple hosts (IP) per port is handled
        for (size_t portIdx = 0; portIdx < ports.size(); ++portIdx)
        {
            int port = ports[portIdx];

            if (portToServerIndex.find(port) != portToServerIndex.end())
            {
                if (portToServerIndex[port] != serverIdx)
                {
                    std::cerr << "ERROR: Port " << port << " is used by multiple server blocks ("
                              << "server " << portToServerIndex[port] + 1 << " and server " << serverIdx + 1 << ")" << std::endl;
                    allValid = false;
                }
                else
                {
                    std::cerr << "WARNING: Duplicate port " << port << " found in server block " << serverIdx + 1 << std::endl;
                }
            }
            else
            {
                portToServerIndex[port] = serverIdx;
            }
        }

        // Second Check: Root directory exists and is accessible
        const std::string &rootDir = serverConfig.getRoot();
        if (!rootDir.empty())
        {
            struct stat statBuf;
            if (stat(rootDir.c_str(), &statBuf) == -1)
            {
                std::cerr << "ERROR: Root directory does not exist: " << rootDir
                          << " (server block " << serverIdx + 1 << ")" << std::endl;
                allValid = false;
            }
            else if (!S_ISDIR(statBuf.st_mode))
            {
                std::cerr << "ERROR: Root path is not a directory: " << rootDir
                          << " (server block " << serverIdx + 1 << ")" << std::endl;
                allValid = false;
            }
            else if (access(rootDir.c_str(), R_OK) == -1)
            {
                std::cerr << "ERROR: Root directory is not readable: " << rootDir
                          << " (server block " << serverIdx + 1 << ")" << std::endl;
                allValid = false;
            }
        }

        // Third Check: Index file exists if specified
        const std::string &indexFile = serverConfig.getIndex();
        if (!indexFile.empty() && !rootDir.empty())
        {
            std::string indexPath = rootDir + "/" + indexFile;
            if (access(indexPath.c_str(), R_OK) != 0)
            {
                std::cerr << "WARNING: Index file not accessible: " << indexPath
                          << " (server block " << serverIdx + 1 << ")" << std::endl;
                // This is a warning, not a fatal error
            }
        }

        // Fourth Check: Valid port range
        for (size_t portIdx = 0; portIdx < ports.size(); ++portIdx)
        {
            int port = ports[portIdx];
            if (port < 1 || port > 65535)
            {
                std::cerr << "ERROR: Invalid port number: " << port
                          << " (server block " << serverIdx + 1 << ")" << std::endl;
                allValid = false;
            }
        }

        if (allValid)
        {
            std::cout << "Server block " << serverIdx + 1 << " passed validation." << std::endl;
        }
    }

    if (allValid)
    {
        std::cout << "All server blocks passed validation." << std::endl;
    }
    else
    {
        std::cerr << "One or more server blocks failed validation." << std::endl;
    }

    return allValid;
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

    host = parser.getServerName();

    std::vector<size_t> candidates;
    for (size_t i = 0; i < _servers.size(); ++i)
    {
        const ServerConfig &serverConfig = _servers[i];
        const std::vector<int> &listenPorts = serverConfig.getListenPorts();
        for (size_t j = 0; j < listenPorts.size(); ++j)
        {
            if (listenPorts[j] == serverPort)
            {
                candidates.push_back(i);
                break; // a server only needs to match the port once
            }
        }
    }

    if (candidates.empty())
    {
        return static_cast<size_t>(-1);
    }

    // If exactly one server listens on the port, return it.
    if (candidates.size() == 1)
    {
        return candidates[0];
    }

    // TODO: Since we already check if there is any duplicate port during validation, we could remove this check, or update it to consider multiple hosts (IP) per port.
    // Multiple servers listen on the same port — prefer server_name match if Host header provided

    if (!host.empty())
    {
        for (size_t idx = 0; idx < candidates.size(); ++idx)
        {
            size_t srvIdx = candidates[idx];
            const std::string &serverName = _servers[srvIdx].getServerName();
            if (iequals(serverName, host))
                return srvIdx;
        }
    }

    // Fallback: return the first candidate (configuration order decides default)
    return candidates[0];
}

size_t HttpServer::getServerMaxBodySize(size_t serverIndex)
{
    if (serverIndex < _servers.size())
    {
        return _servers[serverIndex].getClientMaxBodySize();
    }
    // Return a default value if the server index is invalid
    return 1024 * 1024; // 1 MiB
}