/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseErrorPage.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 15:05:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/20 17:26:59 by pmolzer          ###   ########.fr       */
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
    if (parts.size() < 2) 
    {
        std::string msg = ErrorHandler::makeLocationMsg("error_page requires at least a code and a target", (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
    }
    std::string target = parts.back();
    // Create for loop over each code in parts except the last part (which is the target):
    //     Parse the code 
    //     If invalid, throw an exception for CONFIG_INVALID_DIRECTIVE
    //     Otherwise, set _errorPage to the raget
}
