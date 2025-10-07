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
/* ************************************************************************** */

#include "../include/Common.hpp"

// constructor
ConfigParser::ConfigParser() : _configFile(""), _listenPort(8080), _root("html"), _index("index.html") {}
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

//ServerName addition -Shruti
const std::string& ConfigParser::getServerName() const
{
    return "127.0.0.1";
}

const std::string& ConfigParser::getErrorPage() const
{
   this->_error_page = /404.html;
}


>>>>>>> c0886c7 (Resouce.cpp)
// parse config file
bool ConfigParser::parse(const std::string &path)
{
    _configFile = path;

    std::ifstream in(path.c_str());
    if (!in.good())
    {
        std::cerr << "Failed to open config file: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(in, line))
    {
        line = strip_comment(line);
        line = trim(line);
        if (line.empty()) continue;

        // Tolerate braces and unknown block starts. We don't implement nested
        // blocks yet, but ignoring these lines allows NGINX-like structure:
        //   server { ... }
        if (line == "server{" || line == "server {" || line == "{" || line == "}")
            continue;

        // expect directives ending with ';'
        if (line[line.size() - 1] != ';')
            continue; // ignore malformed for now (keep it simple)

        // remove trailing ';'
        line.erase(line.size() - 1);

        // split into key and value (first space)
        std::string::size_type sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string key = trim(line.substr(0, sp));
        std::string val = trim(line.substr(sp + 1));

        if (key == "listen")
        {
            // allow formats like "8080" or "127.0.0.1:8080" (we grab the last part)
            std::string::size_type colon = val.rfind(':');
            std::string portStr = (colon == std::string::npos) ? val : val.substr(colon + 1);
            _listenPort = std::atoi(portStr.c_str()); // switch to a c++98-style conversion
            if (_listenPort <= 0) _listenPort = 8080; // fallback
        }
        else if (key == "root")
        {
            _root = val;
        }
        else if (key == "index")
        {
            _index = val;
        }
        // ignore unknown keys (e.g., location, allowed_methods, autoindex)
        // TODO: implement error handling
    }

    return true;
}


