
#include "Common.hpp"

void ServerConfig::parseListen(const std::string &val, size_t lineNo)
{
    std::string::size_type colonPos = val.rfind(':');
    std::string portStr = (colonPos == std::string::npos) ? val : val.substr(colonPos + 1);
    int port = std::atoi(portStr.c_str());

    if (port <= 0)
    {
        std::string msg = ErrorHandler::makeLocationMsg(std::string("Invalid port number: ") + portStr,
                                                        (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_PORT, (int)lineNo, this->_configFile);
    }

    // Extract host (IP address) if provided, default to all interfaces
    std::string host = "0.0.0.0";
    if (colonPos != std::string::npos)
    {
        std::string hostStr = trim(val.substr(0, colonPos));
        if (!hostStr.empty())
        {
            host = hostStr;
        }
    }

    // Store as host:port pair (primary storage)
    // this->_listenAddresses.push_back(std::make_pair(host, port));

    // Avoid adding duplicate port entries. If the port is already present, skip it.
	// Might need to be updated if multiple IP is supported
    if (std::find(this->_ports.begin(), this->_ports.end(), port) != this->_ports.end())
    {
        DEBUG_PRINT("Port " << port << " already present; skipping duplicate listen entry");
        return;
    }

    this->_hosts.push_back(host);
    this->_ports.push_back(port);

    DEBUG_PRINT("Applied listen -> " << host << ":" << port);
    // DEBUG_PRINT("Total listen addresses: " << this->_listenAddresses.size());
}
