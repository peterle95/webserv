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

#include "../include/Common.hpp"

// constructor
ConfigParser::ConfigParser() : _configFile(""), _listenPort(8080), _root("html"), _index("index.html"), _lines() {}
// construct from lines
ConfigParser::ConfigParser(const std::vector<std::string>& lines) : _configFile(""), _listenPort(8080), _root("html"), _index("index.html"), _lines()
{
    parseLines(lines);
}
// destructor
ConfigParser::~ConfigParser() {}

// static helper functions, trim whitespace from left
static std::string ltrim(const std::string &s)
{
    std::string::size_type i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r')) 
        i++;
    return s.substr(i);
}

// static helper functions, trim whitespace from right
static std::string rtrim(const std::string &s)
{
    if (s.empty())
        return s;
    std::string::size_type i = s.size();
    while (i > 0 && (s[i-1] == ' ' || s[i-1] == '\t' || s[i-1] == '\r'))
        i--;
    return s.substr(0, i);
}

// static helper functions, trim whitespace from both sides
static std::string trim(const std::string &s)
{ 
    return rtrim(ltrim(s)); 
}

// static helper functions, strip comments
static std::string strip_comment(const std::string &s)
{
    std::string::size_type pos = s.find('#');
    if (pos == std::string::npos)
        return s;
    return s.substr(0, pos);
}

// getters
int ConfigParser::getListenPort() const 
{ 
    return _listenPort; 
}

// getters
const std::string& ConfigParser::getRoot() const 
{ 
    return _root; 
}

// getters
const std::string& ConfigParser::getIndex() const 
{ 
    return _index; 
}

const std::vector<std::string>& ConfigParser::getLines() const
{
    return _lines;
}

// helper: parse from given lines
void ConfigParser::parseLines(const std::vector<std::string>& lines)
{
    this->_lines.clear();

    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::string line = strip_comment(lines[i]);
        line = trim(line);

        if (!line.empty())
            this->_lines.push_back(line);

        if (line.empty())
            continue;

        if (line == "server{" || line == "server {" || line == "{" || line == "}")
            continue;

        if (line[line.size() - 1] != ';')
            continue;

        line.erase(line.size() - 1);

        std::string::size_type sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string key = trim(line.substr(0, sp));
        std::string val = trim(line.substr(sp + 1));

        if (key == "listen")
        {
            std::string::size_type colon = val.rfind(':');
            std::string portStr = (colon == std::string::npos) ? val : val.substr(colon + 1);
            this->_listenPort = std::atoi(portStr.c_str());
            if (this->_listenPort <= 0) 
            {
                std::cerr << "Invalid port number: " << portStr << std::endl;
                this->_listenPort = 8080;
            }
        }
        else if (key == "root")
        {
            this->_root = val;
        }
        else if (key == "index")
        {
            this->_index = val;
        }
        // TODO: add for more directives
    }
}

// parse config file
bool ConfigParser::parse(const std::string &path)
{
    this->_configFile = path;

    std::ifstream in(path.c_str());
    if (!in.good())
    {
        std::cerr << "Failed to open config file: " << path << std::endl;
        return false;
    }

    std::vector<std::string> rawLines;
    std::string line;
    while (std::getline(in, line))
        rawLines.push_back(line);

    parseLines(rawLines);

    return true;
}
