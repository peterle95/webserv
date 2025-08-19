/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseListen.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 15:05:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/19 18:18:21 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../../include/Common.hpp"

void ConfigParser::parseListen(const std::string &val, size_t lineNo)
{
    std::string::size_type colonPos = val.rfind(':');
    std::string portStr = (colonPos == std::string::npos) ? val : val.substr(colonPos + 1);
    this->_listenPort = std::atoi(portStr.c_str());
    if (this->_listenPort <= 0)
    {
        std::string msg = ErrorHandler::makeLocationMsg(std::string("Invalid port number: ") + portStr,
                                                        (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_PORT, (int)lineNo, this->_configFile);
    }
    DEBUG_PRINT("Applied listen -> " << this->_listenPort);
    // If host was provided (e.g., 127.0.0.1:8080), capture it
    if (colonPos != std::string::npos) {
        std::string host = trim(val.substr(0, colonPos));
        if (!host.empty()) {
            this->_host = host;
            DEBUG_PRINT("Applied host -> '" << this->_host << "'");
        }
    }
}
