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

// Ensure a value is provided
static void ensureHasValue(const std::string &val, size_t lineNo, const std::string &cfgFile) {
    if (val.empty()) {
        std::string msg = ErrorHandler::makeLocationMsg("client_max_body_size requires a value", (int)lineNo, cfgFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, cfgFile);
    }
}

// Map a unit character to its multiplier (k, m, g)
static unsigned long unitToMultiplier(char unitRaw, size_t lineNo, const std::string &cfgFile) {
    char unit = static_cast<char>(std::tolower(static_cast<unsigned char>(unitRaw)));
    if (unit == 'k') return 1024UL;
    if (unit == 'm') return 1024UL * 1024UL;
    if (unit == 'g') return 1024UL * 1024UL * 1024UL;
    std::string msg = ErrorHandler::makeLocationMsg(std::string("Unknown size unit: ") + unitRaw, (int)lineNo, cfgFile);
    throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, cfgFile);
}

// If there's a unit suffix, compute multiplier and trimmed number part; otherwise multiplier=1 and number part is val
static void splitValue(const std::string &val, size_t lineNo, const std::string &cfgFile,
                      unsigned long &multiplierOut, std::string &numberPartOut) {
    multiplierOut = 1UL;
    numberPartOut = val;
    if (!val.empty()) {
        char last = val[val.size() - 1];
        if (std::isalpha(static_cast<unsigned char>(last))) {
            multiplierOut = unitToMultiplier(last, lineNo, cfgFile);
            numberPartOut = trim(val.substr(0, val.size() - 1));
        }
    }
}

// Parse the numeric base value and validate
static unsigned long parseBase(const std::string &numberPart, const std::string &originalVal,
                              size_t lineNo, const std::string &cfgFile) {
    char *endptr = NULL;
    errno = 0;
    unsigned long base = std::strtoul(numberPart.c_str(), &endptr, 10);
    if (endptr == NULL || *endptr != '\0' || errno == ERANGE) {
        std::string msg = ErrorHandler::makeLocationMsg(std::string("Invalid size value: ") + originalVal, (int)lineNo, cfgFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, cfgFile);
    }
    return base;
}

void ConfigParser::parseClientMaxBodySize(const std::string &val, size_t lineNo)
{
    // Accept formats like: 1024, 10k, 1m, 1g (case-insensitive); store in bytes
    ensureHasValue(val, lineNo, this->_configFile);

    unsigned long multiplier = 1UL;
    std::string numberPart;
    splitValue(val, lineNo, this->_configFile, multiplier, numberPart);

    unsigned long base = parseBase(numberPart, val, lineNo, this->_configFile);
    unsigned long total = base * multiplier;
    if (base != 0 && total / base != multiplier) {
        std::string msg = ErrorHandler::makeLocationMsg(std::string("Invalid size value: ") + val, (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
    }

    this->_clientMaxBodySize = static_cast<size_t>(total);
    DEBUG_PRINT("Applied client_max_body_size -> " << this->_clientMaxBodySize << " bytes");
}
