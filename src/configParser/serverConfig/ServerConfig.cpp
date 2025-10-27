

#include "Common.hpp"
#include <arpa/inet.h>
#include <sstream>

// constructor
ServerConfig::ServerConfig(const std::string &root, const std::string &index, size_t clientMaxBodySize)
    : _configFile(""), _ports(), _hosts(), _root(root), _index(index), _serverName(""), _errorPage(), _clientMaxBodySize(clientMaxBodySize), _allowedMethods()
{
    // Default host to 127.0.0.1
    if (inet_pton(AF_INET, "127.0.0.1", &_host) != 1) {
        throw std::runtime_error("Failed to set default host");
    }
    // Initialize default listen address
    //_listenAddresses.push_back(std::make_pair("0.0.0.0", 8080));
}

// construct from lines
ServerConfig::ServerConfig(const std::string &root, const std::string &index, size_t clientMaxBodySize, const std::vector<std::string> &lines)
    : _configFile(""), _ports(), _hosts(), _root(root), _index(index), _serverName(""), _errorPage(), _clientMaxBodySize(clientMaxBodySize),
      _allowedMethods()
{
    parse(lines);
}
// destructor
ServerConfig::~ServerConfig() {}

// helper: parse from given lines
// Minimal validation with error throwing for invalid syntax/values
void ServerConfig::parse(const std::vector<std::string> &lines)
{
    DEBUG_PRINT("Parsing server config");
    LocationConfig *currentLocation = NULL;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::string line = lines[i];

        if (line.empty())
        {
            DEBUG_PRINT("Line " << i << " skipped: empty after trim/comment");
            continue;
        }
        // Detect location block
        if ((line.find("location") == 0) && (line.find("{") != std::string::npos))
        {
            size_t bracePos = line.find('{');
            std::string locPath = trim(line.substr(8, bracePos - 8));
            _locations[locPath] = LocationConfig();
            currentLocation = &_locations[locPath];
            currentLocation->path = locPath;
            DEBUG_PRINT("Started location block for path: '" << locPath << "'");
            continue;
        }
        if (line == "}")
        {
            if (currentLocation)
            {
                DEBUG_PRINT("Ended location block for path: '" << currentLocation->path << "'");
                currentLocation = NULL;
            }
            else
            {
                DEBUG_PRINT("Line " << i << " skipped: unmatched '}'");
            }
            continue;
        }

        if (isBlockMarker(line))
        {
            DEBUG_PRINT("Line " << i << " skipped: brace/block marker");
            continue;
        }

        requireSemicolon(line, i + 1);
        line = stripTrailingSemicolon(line);
        DEBUG_PRINT("Line " << i << " directive: '" << line << "'");
        std::string key, val;
        if (!splitKeyVal(line, key, val))
            continue;
        DEBUG_PRINT("Directive key='" << key << "' val='" << val << "'");

        if (currentLocation)
        {
            // Handle location-specific directives
            handleLocationDirective(currentLocation, key, val, i + 1);
            continue;
        }
        handleDirective(key, val, i + 1);
    }
}

const std::map<std::string, LocationConfig> &ServerConfig::getLocations() const
{
    return this->_locations;
}

const std::string &ServerConfig::getErrorPage(int status_code) const
{
    std::map<int, std::string>::const_iterator it = _errorPage.find(status_code);
    if (it != _errorPage.end())
        return it->second;
    static const std::string empty = "";
    return empty;
}

size_t ServerConfig::getClientMaxBodySize() const
{
    return _clientMaxBodySize;
}

// Get first port for backward compatibility
int ServerConfig::getListenPort() const
{
    return _ports.empty() ? 8080 : _ports[0];
}

// Get all ports
const std::vector<int> &ServerConfig::getListenPorts() const
{
    return _ports;
}

// Get all hosts (parallel to ports)
const std::vector<std::string> &ServerConfig::getListenHosts() const
{
    return _hosts;
}

const std::string &ServerConfig::getServerName() const
{
    return _serverName;
}

const std::string &ServerConfig::getRoot() const
{
    return _root;
}
const std::string &ServerConfig::getIndex() const
{
    return _index;
}

in_addr_t ServerConfig::getHost() const
{
    return _host;
}

void ServerConfig::parseHost(const std::string &val, size_t lineNo)
{
    std::string hostStr = val;
    if (hostStr == "localhost") {
        hostStr = "127.0.0.1";
    }

    if (inet_pton(AF_INET, hostStr.c_str(), &_host) != 1) {
        std::stringstream ss;
        ss << "Invalid host format at line " << lineNo;
        throw std::runtime_error(ss.str());
    }
}
// Get all host:port pairs (recommended method)
/* const std::vector<std::pair<std::string, int>> &ServerConfig::getListenAddresses() const
{
    return _listenAddresses;
} */
