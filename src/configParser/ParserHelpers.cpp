/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParserHelpers.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 17:35:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/19 17:42:19 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"
#include "ParserUtils.hpp"

std::string ConfigParser::preprocessLine(const std::string &raw)
{
    return ParserUtils::preprocessLine(raw);
}

bool ConfigParser::isBlockMarker(const std::string &line) const
{
    return ParserUtils::isBlockMarker(line);
}

void ConfigParser::requireSemicolon(const std::string &line, size_t lineNo) const
{
    if (line.empty() || line[line.size() - 1] != ';')
    {
        std::string msg = ErrorHandler::makeLocationMsg("Missing ';' at end of directive", (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_MISSING_SEMICOLON, (int)lineNo, this->_configFile);
    }
}

std::string ConfigParser::stripTrailingSemicolon(const std::string &line) const
{
    return ParserUtils::stripTrailingSemicolon(line);
}

bool ConfigParser::splitKeyVal(const std::string &line, std::string &key, std::string &val) const
{
    return ParserUtils::splitKeyVal(line, key, val);
}

void ConfigParser::handleDirective(const std::string &key, const std::string &val, size_t lineNo)
{
    if (key == "root")
        parseRoot(val, lineNo, &this->_root);
    else if (key == "index")
        parseIndex(val, lineNo, &this->_index);
    else if (key == "client_max_body_size")
        parseClientMaxBodySize(val, lineNo);
    else
    {
        // Unknown directive: ignore non-fatally for now
        DEBUG_PRINT("Unknown directive '" << key << "' (ignored)");
    }
}
