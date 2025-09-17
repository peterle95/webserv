/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequestLine.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 15:41:24 by assistant         #+#    #+#             */
/*   Updated: 2025/09/13 15:41:24 by assistant        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPHeaders.hpp"
#include "HTTPValidation.hpp"
#include "Common.hpp"

HTTPRequestLine::HTTPRequestLine()
    : _isValid(false)
{
    reset();
}

HTTPRequestLine::~HTTPRequestLine()
{
}

/*
Parse the HTTP request line
 
 Parses a request line string into method, path, and version components.
 Validates each component according to HTTP specifications.
 
 requestLine The raw request line string
 return true if parsing was successful, false otherwise
 */
bool HTTPRequestLine::parseRequestLine(const std::string& requestLine)
{
    // Reset previous state and keep the original request line for diagnostics
    reset();
    _rawLine = requestLine;
    
    DEBUG_PRINT("Parsing request line: " << requestLine);
    
    // Check if the line is empty
    if (requestLine.empty())
    {
        setError("Request line is empty");
        return false;
    }
    
    // Create a copy for processing
    std::string line = requestLine;
    trimTrailingCR(line);
    
    // Parse components using stringstream: tokens are whitespace-separated.
    // Expect exactly three tokens: METHOD SP PATH SP VERSION.
    std::istringstream iss(line);
    if (!parseComponents(iss))
        return false;
    
    // Validate each component
    if (!validateMethod(_method))
    {
        setError("Invalid HTTP method: " + _method);
        return false;
    }
    
    if (!validatePath(_path))
    {
        setError("Invalid request path: " + _path);
        return false;
    }
    
    if (!validateVersion(_version))
    {
        setError("Invalid HTTP version: " + _version);
        return false;
    }
    
    _isValid = true;
    DEBUG_PRINT("Successfully parsed request line - Method: " << _method 
                << ", Path: " << _path << ", Version: " << _version);
    
    return true;
}

/*
Parse the three components from the request line

iss Input string stream containing the request line
return true if all three components were extracted, false otherwise
*/
bool HTTPRequestLine::parseComponents(std::istringstream& iss)
{
    // Extract method, path, and version separated by spaces
    if (!(iss >> _method))
    {
        setError("Missing HTTP method");
        return false;
    }
    
    if (!(iss >> _path))
    {
        setError("Missing request path");
        return false;
    }
    
    if (!(iss >> _version)) // Ex: HTTP/1.1
    {
        setError("Missing HTTP version");
        return false;
    }
    
    // Check if there are extra components (which would be invalid)
    std::string extra; // Any extra token means malformed request line
    if (iss >> extra)
    {
        setError("Too many components in request line");
        return false;
    }
    
    return true;
}

// Validate HTTP method
// The method string to validate
//return true if the method is valid, false otherwise
bool HTTPRequestLine::validateMethod(const std::string& method)
{
    if (method.empty())
        return false;
    
    return HTTPValidation::isValidMethod(method);
}

// Validate request path with security checks
// The path string to validate
// return true if the path is valid and safe, false otherwise
bool HTTPRequestLine::validatePath(const std::string& path)
{
    if (path.empty())
        return false;
    
    // We validate only the path component (request-target) here and do not
    // implement query-string parsing or percent-decoding. Those can be
    // layered later if needed by routing.
    return HTTPValidation::isValidPath(path);
}

// Validate HTTP version
// The version string to validate
// return true if the version is supported, false otherwise
bool HTTPRequestLine::validateVersion(const std::string& version)
{
    if (version.empty())
        return false;
    
    return HTTPValidation::isValidVersion(version);
}

// Reset the object to initial state
void HTTPRequestLine::reset()
{
    _method.clear();
    _path.clear();
    _version.clear();
    _rawLine.clear();
    _isValid = false;
    _errorMessage.clear();
}

// Set error state with message
// Error message to set
void HTTPRequestLine::setError(const std::string& message)
{
    _isValid = false;
    _errorMessage = message;
    DEBUG_PRINT("Request line error: " << message);
}

// Remove trailing carriage return character if present
// String to trim
void HTTPRequestLine::trimTrailingCR(std::string& line)
{
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
}