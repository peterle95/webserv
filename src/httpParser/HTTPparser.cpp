/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPparser.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/22 17:34:05 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/22 17:34:05 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// ./src/httpParser/HTTPparser.cpp
#include "../../include/Common.hpp"

HTTPparser::HTTPparser()
    : _state(PARSING_REQUEST_LINE),
      _contentLength(0)
{
    // Default constructor initializes the state and numeric values.
    // String, vector, and map members are default-constructed to an empty state.
}

HTTPparser::~HTTPparser()
{}

// Next Steps:
// 1. Connect HTTP Parser to server logic
// 2. Logic for identification which HTTP version is used (1.0 and 1.1)
// 3. Implement state machine for parsing (see enum in header file). TCP stream might come in chunks
// 4. Use buffer for collect chunks of data from HTTP requests

// Rudimentary parse
// reads the HTTP request from the socket and splits it into request line, headers and body
void HTTPparser::parseRequest(const std::string &rawRequest)
{
    _HTTPrequest = rawRequest; // for debugging/logging
    std::istringstream iss(rawRequest);
    std::string line;

    // 1. Parse request line
    if (std::getline(iss, line)) {
        std::istringstream rl(line);
        rl >> _method >> _path >> _version;
        DEBUG_PRINT("Method: " << _method);
        DEBUG_PRINT("Path:   " << _path);
        DEBUG_PRINT("HTTP:   " << _version);
    }

    // 2. Parse headers until empty line
    while (std::getline(iss, line)) 
    {
        if (line == "\r" || line.empty())
            break;
        // strip trailing \r if present
        if (!line.empty() && line[line.size()-1] == '\r')
            line.erase(line.size()-1);

        std::string::size_type pos = line.find(':');
        if (pos != std::string::npos) 
        {
            std::string key = trim(line.substr(0, pos));
            std::string val = trim(line.substr(pos + 1));
            _headers[key] = val;
            DEBUG_PRINT("Header: [" << key << "] = [" << val << "]");
            if (key == "Content-Length")
                _contentLength = static_cast<size_t>(std::atoi(val.c_str()));
        }
    }

    // 3. Body (for now: just read rest)
    std::ostringstream bodyStream;
    while (std::getline(iss, line))
    {
        bodyStream << line << "\n";
    }
    _body = bodyStream.str();

    if (!_body.empty())
        DEBUG_PRINT("Body: " << _body);
}

const std::string& HTTPparser::getMethod() const
{
    return _method;
}

const std::string& HTTPparser::getPath() const
{
    return _path;
}

const std::string& HTTPparser::getVersion() const
{
    return _version;
}

const std::map<std::string, std::string>& HTTPparser::getHeaders() const
{
    return _headers;
}

const std::string& HTTPparser::getBody() const
{
    return _body;
}

