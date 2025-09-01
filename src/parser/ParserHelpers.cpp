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

#include "../../include/Common.hpp"

std::string ConfigParser::preprocessLine(const std::string &raw)
{
    std::string line = strip_comment(raw);
    line = trim(line);
    return line;
}

bool ConfigParser::isBlockMarker(const std::string &line) const
{
    return (line == "}" || line.find('{') != std::string::npos);
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
    if (!line.empty() && line[line.size() - 1] == ';')
        return line.substr(0, line.size() - 1);
    return line;
}

bool ConfigParser::splitKeyVal(const std::string &line, std::string &key, std::string &val) const
{
    std::string::size_type sp = line.find(' ');
    if (sp == std::string::npos) return false;
    key = trim(line.substr(0, sp));
    val = trim(line.substr(sp + 1));
    return true;
}

void ConfigParser::handleDirective(const std::string &key, const std::string &val, size_t lineNo)
{
    if (key == "listen")
        parseListen(val, lineNo);
    else if (key == "root")
        parseRoot(val, lineNo);
    else if (key == "index")
        parseIndex(val, lineNo);
    else if (key == "server_name")
        parseServerName(val, lineNo);
    else if (key == "client_max_body_size")
        parseClientMaxBodySize(val, lineNo);
    else if (key == "allowed_methods")
        parseAllowedMethods(val, lineNo);
    else if (key == "error_page")
        parseErrorPage(val, lineNo);
    else {
        // Unknown directive: ignore non-fatally for now
        DEBUG_PRINT("Unknown directive '" << key << "' (ignored)");
    }
    // TODO: implement more directives
}
