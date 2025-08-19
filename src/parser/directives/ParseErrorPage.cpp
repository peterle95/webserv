/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseErrorPage.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 15:05:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/19 15:05:00 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../../include/Common.hpp"

void ConfigParser::parseErrorPage(const std::string &val, size_t lineNo)
{
    // Syntax: error_page <code> [code ...] <uri>
    std::istringstream iss(val);
    std::vector<std::string> parts;
    std::string tok;
    while (iss >> tok) parts.push_back(tok);
    if (parts.size() < 2) {
        std::string msg = ErrorHandler::makeLocationMsg("error_page requires at least a code and a target", (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
    }
    std::string target = parts.back();
    for (size_t k = 0; k + 1 < parts.size(); ++k) {
        const std::string &codeStr = parts[k];
        int code = std::atoi(codeStr.c_str());
        if (code <= 0) {
            std::string msg = ErrorHandler::makeLocationMsg(std::string("Invalid status code in error_page: ") + codeStr,
                                                            (int)lineNo, this->_configFile);
            throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
        }
        this->_errorPage[code] = target;
        DEBUG_PRINT("Applied error_page -> " << code << " => '" << target << "'");
    }
}
