/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseAllowedMethods.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 15:05:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/19 17:29:37 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"

void ServerConfig::parseAllowedMethods(const std::string &val, size_t lineNo, std::set<std::string> *allowedMethods)
{
    // Split by spaces and validate methods
    std::istringstream iss(val);
    std::string tok;
    while (iss >> tok)
    {
        if (tok != "GET" && tok != "POST" && tok != "DELETE")
        {
            std::string msg = ErrorHandler::makeLocationMsg(std::string("Unsupported method in allowed_methods: ") + tok,
                                                            (int)lineNo, this->_configFile);
            throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
        }
        allowedMethods->insert(tok);
        DEBUG_PRINT("Added allowed_method -> '" << tok << "'");
    }
    if (allowedMethods->empty())
    {
        std::string msg = ErrorHandler::makeLocationMsg("allowed_methods requires at least one method", (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
    }
}
