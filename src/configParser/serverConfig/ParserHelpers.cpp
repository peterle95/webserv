

#include "Common.hpp"
#include "ParserUtils.hpp"

std::string ServerConfig::preprocessLine(const std::string &raw)
{
    return ParserUtils::preprocessLine(raw);
}

bool ServerConfig::isBlockMarker(const std::string &line) const
{
    return ParserUtils::isBlockMarker(line);
}

void ServerConfig::requireSemicolon(const std::string &line, size_t lineNo) const
{
    if (line.empty() || line[line.size() - 1] != ';')
    {
        std::string msg = ErrorHandler::makeLocationMsg("Missing ';' at end of directive", (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_MISSING_SEMICOLON, (int)lineNo, this->_configFile);
    }
}

std::string ServerConfig::stripTrailingSemicolon(const std::string &line) const
{
    return ParserUtils::stripTrailingSemicolon(line);
}

bool ServerConfig::splitKeyVal(const std::string &line, std::string &key, std::string &val) const
{
    return ParserUtils::splitKeyVal(line, key, val);
}

void ServerConfig::handleDirective(const std::string &key, const std::string &val, size_t lineNo)
{
    if (key == "listen")
        parseListen(val, lineNo);
    else if (key == "host")
        parseHost(val, lineNo);
    else if (key == "root")
        parseRoot(val, lineNo, &this->_root);
    else if (key == "index")
        parseIndex(val, lineNo, &this->_index);
    else if (key == "server_name")
        parseServerName(val, lineNo);
    else if (key == "client_max_body_size")
        parseClientMaxBodySize(val, lineNo);
    else if (key == "allowed_methods")
        parseAllowedMethods(val, lineNo, &this->_allowedMethods);
    else if (key == "error_page")
        parseErrorPage(val, lineNo);
    else
    {
        // Unknown directive: ignore non-fatally for now
        DEBUG_PRINT("Unknown directive '" << key << "' (ignored)");
    }
    // TODO: implement more directives
}
