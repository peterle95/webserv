/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseClientMaxBodySize.cpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 15:05:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/19 15:05:00 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../../include/Common.hpp"

void ConfigParser::parseClientMaxBodySize(const std::string &val, size_t lineNo)
{
    // Accept formats like: 1024, 10k, 1m, 1g (case-insensitive); store in bytes
    if (val.empty()) {
        std::string msg = ErrorHandler::makeLocationMsg("client_max_body_size requires a value", (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
    }
    unsigned long long multiplier = 1ULL;
    char last = val[val.size() - 1];
    std::string numberPart = val;
    if (std::isalpha(static_cast<unsigned char>(last))) {
        char unit = static_cast<char>(std::tolower(static_cast<unsigned char>(last)));
        if (unit == 'k') multiplier = 1024ULL;
        else if (unit == 'm') multiplier = 1024ULL * 1024ULL;
        else if (unit == 'g') multiplier = 1024ULL * 1024ULL * 1024ULL;
        else {
            std::string msg = ErrorHandler::makeLocationMsg(std::string("Unknown size unit: ") + last, (int)lineNo, this->_configFile);
            throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
        }
        numberPart = val.substr(0, val.size() - 1);
        numberPart = trim(numberPart);
    }
    char* endptr = NULL;
    unsigned long long base = std::strtoull(numberPart.c_str(), &endptr, 10);
    if (endptr == NULL || *endptr != '\0') {
        std::string msg = ErrorHandler::makeLocationMsg(std::string("Invalid size value: ") + val, (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
    }
    unsigned long long total = base * multiplier;
    this->_clientMaxBodySize = static_cast<size_t>(total);
    DEBUG_PRINT("Applied client_max_body_size -> " << this->_clientMaxBodySize << " bytes");
}
