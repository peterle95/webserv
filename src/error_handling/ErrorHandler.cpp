/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorHandler.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/18 11:14:28 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/18 11:46:35 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/ErrorHandler.hpp"
#include <sstream>

// Construct an exception with just message and code
ErrorHandler::Exception::Exception(const std::string &message, int code)
  : _msg(message), _code(code), _line(-1), _file("") {}

// Construct an exception with full location context
ErrorHandler::Exception::Exception(const std::string &message, int code, int line, const std::string &file)
  : _msg(message), _code(code), _line(line), _file(file) {}

ErrorHandler::Exception::~Exception() throw() {}

const char* ErrorHandler::Exception::what() const throw()
{
    return _msg.c_str();
}

// Helper: format a location-aware prefix
std::string ErrorHandler::makeLocationMsg(const std::string &prefix, int line, const std::string &file)
{
    std::ostringstream oss;
    oss << prefix;
    if (!file.empty()) {
        oss << " [file: " << file << "]";
    }
    if (line >= 0) {
        oss << " [line: " << line << "]";
    }
    return oss.str();
}

const char* ErrorHandler::codeToString(int code)
{
    switch (code) {
        case CONFIG_FILE_NOT_FOUND:   
            return "CONFIG_FILE_NOT_FOUND";
        case CONFIG_INVALID_PORT:     
            return "CONFIG_INVALID_PORT";
        case CONFIG_MISSING_SEMICOLON:
            return "CONFIG_MISSING_SEMICOLON";
        case CONFIG_INVALID_DIRECTIVE:
            return "CONFIG_INVALID_DIRECTIVE";
        case CONFIG_UNKNOWN_DIRECTIVE:
            return "CONFIG_UNKNOWN_DIRECTIVE";
        default:                      
            return "UNKNOWN_ERROR_CODE";
    }
}