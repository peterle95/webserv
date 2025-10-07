/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 17:38:04 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/19 17:38:04 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"

// constructor
ConfigParser::ConfigParser()
    : _configFile(""), _listenPort(8080), _root("html"), _index("index.html"), _serverName(""), _errorPage(), _clientMaxBodySize(1024 * 1024) // default 1 MiB
      ,
      _host(""), _allowedMethods(), _lines()
{
}

// construct from lines
ConfigParser::ConfigParser(const std::vector<std::string> &lines)
    : _configFile(""), _listenPort(8080), _root("html"), _index("index.html"), _serverName(""), _errorPage(), _clientMaxBodySize(1024 * 1024) // default 1 MiB
      ,
      _host(""), _allowedMethods(), _lines()
{
    parseLines(lines);
}
// destructor
ConfigParser::~ConfigParser() {}

// helper: parse from given lines
// Minimal validation with error throwing for invalid syntax/values
void ConfigParser::parseLines(const std::vector<std::string>& lines)
{
    this->_lines.clear();
    LocationConfig *currentLocation = NULL;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::string raw = lines[i];
        DEBUG_PRINT("Line " << i << " raw: '" << raw << "'");
        std::string line = preprocessLine(raw);
        DEBUG_PRINT("Line " << i << " preprocessed: '" << line << "'");

        // if (!line.empty())
        //     this->_lines.push_back(line);

        if (line.empty())
        {
            DEBUG_PRINT("Line " << i << " skipped: empty after trim/comment");
            continue;
        }
        // Detect location block
        if ((line.find("location") == 0) && (line.find("{") != std::string::npos))
        {
            size_t bracePos = line.find('{');
            std::string locPath = trim(line.substr(8, bracePos - 8));
            _locations[locPath] = LocationConfig();
            currentLocation = &_locations[locPath];
            currentLocation->path = locPath;
            DEBUG_PRINT("Started location block for path: '" << locPath << "'");
            continue;
        }
        if (line == "}")
        {
            if (currentLocation)
            {
                DEBUG_PRINT("Ended location block for path: '" << currentLocation->path << "'");
                currentLocation = NULL;
            }
            else
            {
                DEBUG_PRINT("Line " << i << " skipped: unmatched '}'");
            }
            continue;
        }

        if (isBlockMarker(line)) {
            DEBUG_PRINT("Line " << i << " skipped: brace/block marker");
            continue;
        }

        requireSemicolon(line, i + 1);
        line = stripTrailingSemicolon(line);

        std::string key, val;
        if (!splitKeyVal(line, key, val))
            continue;
        DEBUG_PRINT("Directive key='" << key << "' val='" << val << "'");

        if (currentLocation)
        {
            // Handle location-specific directives
            handleLocationDirective(currentLocation, key, val, i + 1);
            continue;
        }
        handleDirective(key, val, i + 1);
    }
}

// parse config file
bool ConfigParser::parse(const std::string &path)
{
    this->_configFile = path;
    DEBUG_PRINT("Opening config file: '" << path << "'");

    try {
        std::ifstream in(path.c_str());
        if (!in.good())
        {
            std::string msg = ErrorHandler::makeLocationMsg(std::string("Failed to open config file: ") + path, -1, path);
            throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_FILE_NOT_FOUND);
        }

        std::vector<std::string> rawLines;
        std::string line;
        while (std::getline(in, line))
            rawLines.push_back(line);
        DEBUG_PRINT("Read " << rawLines.size() << " lines from file");

        parseLines(rawLines);
        DEBUG_PRINT("Finished parsing file: '" << path << "'");
        return true;
    }
    catch (const ErrorHandler::Exception &e) {
        std::cerr << "[CONFIG ERROR] (" << ErrorHandler::codeToString(e.code()) << ") " << e.what() << std::endl;
        return false;
    }
}

const std::string& getErrorPage()
{
     return (_errorPage = "/404.html");
}
const std::map<std::string, LocationConfig> &ConfigParser::getLocations() const
{
    return this->_locations;

}






