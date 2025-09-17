/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPHeaders.cpp                                   :+:      :+:    :+:   */
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
#include <algorithm>

HTTPHeaders::HTTPHeaders()
    : _contentLength(0), _hasContentLength(false), _isChunked(false), _isValid(false)
{
    reset();
}

HTTPHeaders::~HTTPHeaders()
{
}

// Parse headers from input stream until empty line is found
// Reads header lines from the input stream, validates them, and stores
// them in the internal map. Stops when an empty line is encountered.
// iss: Input string stream containing header lines
// return true if parsing was successful, false otherwise
bool HTTPHeaders::parseHeaders(std::istringstream& iss)
{
    // Reset any previous state. We'll accumulate headers in _headers map.
    reset();
    std::string line;
    
    DEBUG_PRINT("Starting header parsing");
    
    // Parse each header line until empty line
    while (std::getline(iss, line))
    {
        // Remove trailing CR if present
        trimTrailingCR(line);
        
        // Empty line indicates end of headers
        if (isEmptyLine(line))
        {
            DEBUG_PRINT("Found empty line, headers complete");
            break;
        }
        
        // Parse this header line
        if (!parseHeaderLine(line))
        {
            DEBUG_PRINT("Failed to parse header line: " << line);
            return false;
        }
    }
    
    // Validate the parsed headers for conflicts and basic consistency.
    // Example: both Content-Length and Transfer-Encoding present ->
    // Transfer-Encoding takes precedence.
    if (!validateHeaders())
        return false;
    
    _isValid = true;
    DEBUG_PRINT("Successfully parsed " << _headers.size() << " headers");
    
    return true;
}

// Parse a single header line
// Parses a line in the format "name: value" and adds it to the headers map.
// Validates both name and value according to HTTP specifications.
// line: The header line to parse
// return true if parsing was successful, false otherwise
bool HTTPHeaders::parseHeaderLine(const std::string& line)
{
    // Find the colon separator
    std::string::size_type colonPos = line.find(':');
    if (colonPos == std::string::npos)
    {
        setError("Invalid header format, missing colon: " + line);
        return false;
    }
    
    // Extract and trim name and value
    // The value portion may have leading spaces which we trim.
    std::string name = HTTPValidation::trim(line.substr(0, colonPos));
    std::string value = HTTPValidation::trim(line.substr(colonPos + 1));
    
    // Validate header name
    if (!HTTPValidation::isValidHeaderName(name))
    {
        setError("Invalid header name: " + name);
        return false;
    }
    
    // Validate header value
    if (!HTTPValidation::isValidHeaderValue(value))
    {
        setError("Invalid header value for " + name + ": " + value);
        return false;
    }
    
    // Add the header
    return addHeader(name, value);
}

// Add a header to the collection
// name: Header name
// value: Header value
// return true if header was added successfully, false otherwise
bool HTTPHeaders::addHeader(const std::string& name, const std::string& value)
{
    if (name.empty())
        return false;
    
    std::string lowerName = toLowercase(name);
    
    // Store original case for potential future use
    _originalKeys[lowerName] = name;
    
    // Store the header with lowercase key for case-insensitive lookup
    _headers[lowerName] = value;
    
    // Process special headers that need additional parsing
    processSpecialHeaders(lowerName, value);
    
    DEBUG_PRINT("Added header: [" << name << "] = [" << value << "]");
    
    return true;
}

/**
 * @brief Process special headers that require additional handling
 * 
 * @param lowerName Header name in lowercase
 * @param value Header value
 */
void HTTPHeaders::processSpecialHeaders(const std::string& lowerName, const std::string& value)
{
    if (lowerName == "content-length")
    {
        _hasContentLength = parseContentLength(value);
    }
    else if (lowerName == "transfer-encoding")
    {
        _transferEncoding = value;
        parseTransferEncoding(value);
    }
    else if (lowerName == "content-type")
    {
        _contentType = value;
    }
    else if (lowerName == "host")
    {
        _host = value;
    }
    else if (lowerName == "connection")
    {
        _connection = value;
    }
}

/**
 * @brief Parse Content-Length header value
 * 
 * @param value The Content-Length value to parse
 * @return true if parsing was successful, false otherwise
 */
bool HTTPHeaders::parseContentLength(const std::string& value)
{
    size_t length;
    if (HTTPValidation::isValidContentLength(value, length))
    {
        _contentLength = length;
        return true;
    }
    else
    {
        setError("Invalid Content-Length value: " + value);
        return false;
    }
}

/**
 * @brief Parse Transfer-Encoding header value
 * 
 * @param value The Transfer-Encoding value to parse
 * @return true if parsing was successful, false otherwise
 */
bool HTTPHeaders::parseTransferEncoding(const std::string& value)
{
    // Convert to lowercase for comparison
    std::string lowerValue = toLowercase(value);
    
    // Check if chunked encoding is specified
    _isChunked = (lowerValue.find("chunked") != std::string::npos);
    
    return true;
}

/**
 * @brief Check if a header exists (case-insensitive)
 * 
 * @param name Header name to check
 * @return true if header exists, false otherwise
 */
bool HTTPHeaders::hasHeader(const std::string& name) const
{
    std::string lowerName = toLowercase(name);
    return _headers.find(lowerName) != _headers.end();
}

/**
 * @brief Get a header value (case-insensitive)
 * 
 * @param name Header name to get
 * @return Header value, or empty string if not found
 */
std::string HTTPHeaders::getHeader(const std::string& name) const
{
    std::string lowerName = toLowercase(name);
    std::map<std::string, std::string>::const_iterator it = _headers.find(lowerName);
    if (it != _headers.end())
        return it->second;
    return "";
}

/**
 * @brief Remove a header (case-insensitive)
 * 
 * @param name Header name to remove
 */
void HTTPHeaders::removeHeader(const std::string& name)
{
    std::string lowerName = toLowercase(name);
    _headers.erase(lowerName);
    _originalKeys.erase(lowerName);
}

/**
 * @brief Validate all parsed headers
 * 
 * @return true if all headers are valid, false otherwise
 */
bool HTTPHeaders::validateHeaders() const
{
    // Check for conflicting headers
    if (_hasContentLength && _isChunked)
    {
        // According to HTTP/1.1 spec, if both are present, Transfer-Encoding takes precedence
        DEBUG_PRINT("Warning: Both Content-Length and Transfer-Encoding: chunked present");
    }
    
    // Additional validation could be added here
    // For example: checking for required headers, header value constraints, etc.
    
    return true;
}

/**
 * @brief Reset all header data to initial state
 */
void HTTPHeaders::reset()
{
    _headers.clear();
    _originalKeys.clear();
    _contentLength = 0;
    _hasContentLength = false;
    _isChunked = false;
    _contentType.clear();
    _host.clear();
    _connection.clear();
    _transferEncoding.clear();
    _isValid = false;
    _errorMessage.clear();
}

/**
 * @brief Set error state with message
 * 
 * @param message Error message to set
 */
void HTTPHeaders::setError(const std::string& message)
{
    _isValid = false;
    _errorMessage = message;
    DEBUG_PRINT("Header parsing error: " << message);
}

/**
 * @brief Convert string to lowercase
 * 
 * @param str String to convert
 * @return Lowercase version of the string
 */
std::string HTTPHeaders::toLowercase(const std::string& str) const
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

/**
 * @brief Remove trailing carriage return if present
 * 
 * @param line String to trim
 */
void HTTPHeaders::trimTrailingCR(std::string& line)
{
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
}

/**
 * @brief Check if line is empty (considering just spaces and CR)
 * 
 * @param line Line to check
 * @return true if line is effectively empty, false otherwise
 */
bool HTTPHeaders::isEmptyLine(const std::string& line) const
{
    return line.empty() || line == "\r";
}