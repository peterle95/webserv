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
#include "Common.hpp"
#include "HTTPBody.hpp"

HTTPparser::HTTPparser()
    : _state(PARSING_REQUEST_LINE), _isValid(false)
{
    reset();
}

HTTPparser::~HTTPparser()
{}

/**
 * @brief Parse the HTTP request line using the HTTPRequestLine module
 * 
 * @param iss Input string stream containing the request
 * @return true if parsing was successful, false otherwise
 */
bool HTTPparser::parseRequestLine(std::istringstream& iss)
{
    std::string line;
    
    // Get the first line (request line)
    if (!std::getline(iss, line))
    {
        setError("No request line found", "400");
        return false;
    }
    
    // Use HTTPRequestLine module to parse the line
    if (!_requestLine.parseRequestLine(line))
    {
        setError("Invalid request line: " + _requestLine.getErrorMessage(), "400");
        return false;
    }
    
    DEBUG_PRINT("Successfully parsed request line - Method: " << _requestLine.getMethod() 
                << ", Path: " << _requestLine.getPath() << ", Version: " << _requestLine.getVersion());
    
    return true;
}

/**
 * @brief Parse HTTP headers using the HTTPHeaders module
 * 
 * @param iss Input string stream containing the headers section
 * @return true if parsing was successful, false otherwise
 */
bool HTTPparser::parseHeaders(std::istringstream& iss)
{
    // Use HTTPHeaders module to parse all headers
    if (!_headers.parseHeaders(iss))
    {
        setError("Invalid headers: " + _headers.getErrorMessage(), "400");
        return false;
    }
    
    DEBUG_PRINT("Successfully parsed " << _headers.getHeaderCount() << " headers");
    
    // Log important headers for debugging
    if (_headers.hasContentLength())
        DEBUG_PRINT("Content-Length: " << _headers.getContentLength());
    if (_headers.isChunked())
        DEBUG_PRINT("Transfer-Encoding: chunked detected");
    if (!_headers.getHost().empty())
        DEBUG_PRINT("Host: " << _headers.getHost());
    
    return true;
}

/**
 * @brief Parse HTTP body with support for Content-Length and chunked encoding
 * 
 * Handles three cases:
 * - Transfer-Encoding: chunked
 * - Content-Length: N
 * - No explicit length (reads remaining stream)
 * 
 * @param iss Input string stream containing the body section
 * @return true if parsing was successful, false otherwise
 */
bool HTTPparser::parseBody(std::istringstream& iss)
{
    // Delegate body parsing to HTTPBody for modularity
    setState(_headers.isChunked() ? PARSING_CHUNKED_BODY : PARSING_BODY);

    HTTPBody bodyParser;
    if (!bodyParser.parse(iss, _headers, _requestLine.getMethod()))
    {
        setError(bodyParser.getErrorMessage(), "400");
        return false;
    }
    _body = bodyParser.getBody();
    if (!_body.empty())
        DEBUG_PRINT("Parsed body with " << _body.length() << " bytes");
    return true;
}






/**
 * @brief Main method to parse an HTTP request using modular components
 * 
 * This method orchestrates the parsing process using specialized classes:
 * - HTTPRequestLine for parsing the first line
 * - HTTPHeaders for parsing header section
 * - Simple body parsing (to be enhanced later)
 * 
 * The method operates as a state machine to handle the parsing flow.
 * 
 * @param rawRequest The raw HTTP request string to parse
 * @return true if parsing was successful, false otherwise
 */
bool HTTPparser::parseRequest(const std::string &rawRequest)
{
    // Reset parser state
    reset();
    _HTTPrequest = rawRequest; // Store for debugging/logging

    DEBUG_PRINT("=== Starting HTTP Request Parsing ===");
    DEBUG_PRINT("Raw Request (first 200 chars): " << rawRequest.substr(0, 200));
    
    std::istringstream iss(rawRequest);
    
    // State machine implementation
    setState(PARSING_REQUEST_LINE);
    
    // Step 1: Parse request line
    if (_state == PARSING_REQUEST_LINE)
    {
        if (!parseRequestLine(iss))
        {
            setState(ERROR);
            return false;
        }
        setState(PARSING_HEADERS);
    }
    
    // Step 2: Parse headers
    if (_state == PARSING_HEADERS)
    {
        if (!parseHeaders(iss))
        {
            setState(ERROR);
            return false;
        }
        setState(PARSING_BODY);
    }

    // Step 3: Parse body (if present)
    if (_state == PARSING_BODY)
    {
        if (!parseBody(iss))
        {
            setState(ERROR);
            return false;
        }
        setState(PARSING_COMPLETE);
    }
    
    // Final state check
    if (_state == PARSING_COMPLETE)
    {
        _isValid = true;
        DEBUG_PRINT("=== HTTP Request Parsing Complete ===" );
        DEBUG_PRINT("Method: " << getMethod() << ", Path: " << getPath() 
                    << ", Version: " << getVersion());
        DEBUG_PRINT("Headers: " << getHeaders().size() << " total");
        if (!_body.empty())
            DEBUG_PRINT("Body: " << _body.length() << " characters");
        return true;
    }
    else if (_state == ERROR)
    {
        DEBUG_PRINT("=== HTTP Request Parsing Failed ===" );
        DEBUG_PRINT("Error: " << _errorMessage);
        return false;
    }
    
    // Should not reach here
    setError("Unexpected parser state", "500");
    return false;
}

