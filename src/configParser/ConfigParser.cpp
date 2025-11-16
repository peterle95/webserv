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
    : _configFile(""), _root("html"), _index("index.html"), _clientMaxBodySize(1024 * 1024) // default 1 MiB
      ,
      _servers(),
      _lines()
{
}

 ConfigParser::ConfigParser(ConfigParser const &other)
    : _configFile(other._configFile),
      _root(other._root),
      _index(other._index),
      _clientMaxBodySize(other._clientMaxBodySize),
      _servers(other._servers),
      _lines(other._lines)
{
}

// construct from lines
ConfigParser::ConfigParser(const std::vector<std::string> &lines)
    : _configFile(""), _root("html"), _index("index.html"), _clientMaxBodySize(1024 * 1024) // default 1 MiB
      ,
      _servers(),
      _lines()
{
    parseLines(lines);
}
// destructor
ConfigParser::~ConfigParser() {}

// helper: parse from given lines
// Minimal validation with error throwing for invalid syntax/values
void ConfigParser::parseLines(const std::vector<std::string> &lines)
{
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::string raw = lines[i];
        std::string line = preprocessLine(raw);

        if (line.empty())
            continue;

        // Detect server block
        if ((line.find("server") == 0) && (line.find("{") != std::string::npos))
        {
            DEBUG_PRINT("Line " << i << " found: server block");
            std::vector<std::string> serverBlockLines;
            int braceCount = 1; // We already found the opening brace
            ++i;                // Move to the next line after "server {"

            // Collect all lines until matching closing brace
            while (i < lines.size() && braceCount > 0)
            {
                std::string currentLine = preprocessLine(lines[i]);

                // Skip empty lines but still count braces in non-empty lines
                if (!currentLine.empty())
                {
                    // Count opening and closing braces
                    for (size_t j = 0; j < currentLine.length(); ++j)
                    {
                        if (currentLine[j] == '{')
                            braceCount++;
                        else if (currentLine[j] == '}')
                            braceCount--;
                    }

                    // Only add line if we haven't reached the final closing brace
                    if (braceCount > 0)
                    {
                        serverBlockLines.push_back(currentLine);
                    }
                }
                ++i;
            }

            // Check if we properly closed the server block
            if (braceCount != 0)
            {
                // For now, just print debug message instead of throwing
                DEBUG_PRINT("Warning: Unmatched braces in server block at line " << i);
            }

            // Process the server block lines
            ServerConfig serverConfig(_root, _index, _clientMaxBodySize);
            serverConfig.parse(serverBlockLines);
            _servers.push_back(serverConfig);

            --i; // Adjust index since the outer loop will increment it
            continue;
        }

        if (isBlockMarker(line))
            continue;

        requireSemicolon(line, i + 1);
        line = stripTrailingSemicolon(line);

        std::string key, val;
        if (!splitKeyVal(line, key, val))
            continue;
        handleDirective(key, val, i + 1);
    }
}

// parse config file
bool ConfigParser::parse(const std::string &path)
{
    this->_configFile = path;

    try
    {
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

        parseLines(rawLines);
        return true;
    }
    catch (const ErrorHandler::Exception &e)
    {
        std::cerr << "[CONFIG ERROR] (" << ErrorHandler::codeToString(e.code()) << ") " << e.what() << std::endl;
        return false;
    }
}

size_t ConfigParser::getClientMaxBodySize() const
{
    return _clientMaxBodySize;
}

const std::vector<ServerConfig> &ConfigParser::getServers() const
{
    return _servers;
}
