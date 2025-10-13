/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseClientMaxBodySize.cpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 15:05:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/21 15:21:06 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"

void ConfigParser::parseClientMaxBodySize(const std::string &val, size_t lineNo)
{
    if (val.empty())
    {
        std::string msg = ErrorHandler::makeLocationMsg("Missing value for client_max_body_size directive",
                                                        (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
    }

    // Accept formats like: 1024, 10k, 1m, 1g (case-insensitive); store in bytes
    // Ensure a value exists (error if missing)

    // Split the input into a number part and a multiplier (based on suffix like k, m, or g)

    // Parse the numeric value, validate

    // Check for overflow by confirming reverse calculation is consistent

    // Store the final byte value in the config struct

    // Optionally print debug info
    std::string value = val;
    long size;
    if(value[val.size() - 1] >= '0' && value[val.size() - 1] <= '9')
    {
        value = val.substr(0, val.size() - 1);
        size = std::atol(value.c_str());
    }
    else if(value[val.size() - 1] == 'k')
    {
        value = value.substr(0, val.size() - 2);
        size =std::atol(value.c_str());
        size *=  1024;

    }
    else if(value[val.size() - 1] == 'm')
    {
        value = value.substr(0, val.size() - 2);
        size = std::atol(value.c_str());
        size *=  1024 * 1024;
    }
    else if(value[val.size() - 1] == 'g')
    {
        value = value.substr(0, val.size() - 2);
        size = std::atol(value.c_str());
        size *=  1024 * 1024 * 1024;
    }
    else
    {
        std::string msg = ErrorHandler::makeLocationMsg("Invalid suffix for client_max_body_size directive",
                                                        (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
    }

    if(size < 0)
    {
        std::string msg = ErrorHandler::makeLocationMsg("Negative value for client_max_body_size directive",
                                                        (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
    }
    _clientMaxBodySize = size;
    DEBUG_PRINT("Set client_max_body_size to " << _clientMaxBodySize << " bytes");
    
}
