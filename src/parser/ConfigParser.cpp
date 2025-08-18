/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 13:21:29 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/13 13:21:29 by pmolzer          ###   ########.fr       */
/*                                                                            */

#include "../../include/Common.hpp"

// constructor
ConfigParser::ConfigParser() : _configFile(""), _listenPort(8080), _root("html"), _index("index.html"), _lines() {}

// construct from lines
ConfigParser::ConfigParser(const std::vector<std::string>& lines) : _configFile(""), _listenPort(8080), _root("html"), _index("index.html"), _lines()
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

    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::string raw = lines[i];
        DEBUG_PRINT("Line " << i << " raw: '" << raw << "'");
        std::string line = strip_comment(raw);
        line = trim(line);
        DEBUG_PRINT("Line " << i << " trimmed: '" << line << "'");

        if (!line.empty())
            this->_lines.push_back(line);

        if (line.empty()) {
            DEBUG_PRINT("Line " << i << " skipped: empty after trim/comment");
            continue;
        }

        if (line == "server{" || line == "server {" || line == "{" || line == "}") {
            DEBUG_PRINT("Line " << i << " skipped: brace/block marker");
            continue;
        }

        if (line[line.size() - 1] != ';') {
            // Missing semicolon is a syntax error
            std::string msg = ErrorHandler::makeLocationMsg("Missing ';' at end of directive", (int)i + 1, this->_configFile);
            throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_MISSING_SEMICOLON, (int)i + 1, this->_configFile);
        }

        line.erase(line.size() - 1);

        std::string::size_type sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string key = trim(line.substr(0, sp));
        std::string val = trim(line.substr(sp + 1));
        DEBUG_PRINT("Directive key='" << key << "' val='" << val << "'");

        if (key == "listen")
        {
            std::string::size_type colon = val.rfind(':');
            std::string portStr = (colon == std::string::npos) ? val : val.substr(colon + 1);
            this->_listenPort = std::atoi(portStr.c_str());
            if (this->_listenPort <= 0)
            {
                std::string msg = ErrorHandler::makeLocationMsg(std::string("Invalid port number: ") + portStr,
                                                                (int)i + 1, this->_configFile);
                throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_PORT, (int)i + 1, this->_configFile);
            }
            DEBUG_PRINT("Applied listen -> " << this->_listenPort);
        }
        else if (key == "root")
        {
            this->_root = val;
            DEBUG_PRINT("Applied root -> '" << this->_root << "'");
        }
        else if (key == "index")
        {
            this->_index = val;
            DEBUG_PRINT("Applied index -> '" << this->_index << "'");
        }
        else {
            // Keep unknown directives as non-fatal for now (skeleton stage)
            DEBUG_PRINT("Unknown directive '" << key << "' (ignored)");
        }
        // TODO: add for more directives
    }
    DEBUG_PRINT("parseLines complete: listen=" << this->_listenPort << ", root='" << this->_root << "', index='" << this->_index << "'");
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
