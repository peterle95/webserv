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
    // Methods like GET/HEAD/DELETE typically do not include a body
    // but RFC allows it; we simply parse if present.
    if (_headers.isChunked())
    {
        DEBUG_PRINT("Parsing body with Transfer-Encoding: chunked");
        setState(PARSING_CHUNKED_BODY);
        if (!parseChunkedBody(iss))
            return false;
        return true;
    }

    if (_headers.hasContentLength())
    {
        size_t len = _headers.getContentLength();
        DEBUG_PRINT("Parsing body with Content-Length: " << len);
        setState(PARSING_BODY);
        if (!parseFixedLengthBody(iss, len))
            return false;
        return true;
    }

    // No length provided: read remaining bytes as-is
    DEBUG_PRINT("No Content-Length or Transfer-Encoding provided; reading remaining stream as body");
    setState(PARSING_BODY);
    std::ostringstream bodyStream;
    bodyStream << iss.rdbuf();
    _body = bodyStream.str();
    if (!_body.empty())
        DEBUG_PRINT("Parsed body with " << _body.length() << " bytes");
    return true;
}

/**
 * @brief Read exactly 'length' bytes from stream into 'out'
 */
bool HTTPparser::readExact(std::istringstream& iss, size_t length, std::string& out)
{
    if (length == 0)
    {
        out.clear();
        return true;
    }
    std::vector<char> buf(length);
    iss.read(&buf[0], static_cast<std::streamsize>(length));
    std::streamsize got = iss.gcount();
    if (static_cast<size_t>(got) != length)
        return false;
    out.assign(&buf[0], &buf[0] + length);
    return true;
}

/**
 * @brief Consume an expected CRLF from the stream
 */
bool HTTPparser::readCRLF(std::istringstream& iss)
{
    char cr = 0, lf = 0;
    if (!iss.get(cr)) return false;
    if (!iss.get(lf)) return false;
    return (cr == '\r' && lf == '\n');
}

/**
 * @brief Parse a single chunk-size line and output its numeric value
 */
bool HTTPparser::parseChunkSizeLine(std::istringstream& iss, size_t& outSize)
{
    std::string line;
    if (!std::getline(iss, line))
    {
        setError("Unexpected end of input while reading chunk size", "400");
        return false;
    }
    trimTrailingCR(line);

    // Remove chunk extensions if present
    std::string::size_type scPos = line.find(';');
    if (scPos != std::string::npos)
        line = line.substr(0, scPos);

    // Trim whitespace
    line = HTTPValidation::trim(line);
    if (line.empty())
    {
        setError("Empty chunk size line", "400");
        return false;
    }

    // Validate hex digits and parse
    for (size_t i = 0; i < line.size(); ++i)
    {
        if (!std::isxdigit(static_cast<unsigned char>(line[i])))
        {
            setError("Invalid chunk size: non-hex character found", "400");
            return false;
        }
    }

    unsigned long parsed = 0;
    std::istringstream hexss(line);
    hexss >> std::hex >> parsed;
    if (hexss.fail())
    {
        setError("Failed to parse chunk size", "400");
        return false;
    }
    outSize = static_cast<size_t>(parsed);
    DEBUG_PRINT("Chunk size: " << outSize);
    return true;
}

/**
 * @brief Parse body with Transfer-Encoding: chunked
 */
bool HTTPparser::parseChunkedBody(std::istringstream& iss)
{
    _body.clear();
    while (true)
    {
        setState(PARSING_CHUNK_SIZE);
        size_t chunkSize = 0;
        if (!parseChunkSizeLine(iss, chunkSize))
            return false;

        if (chunkSize == 0)
        {
            // End of chunks
            // Consume the CRLF that terminates the zero-length chunk data
            if (!readCRLF(iss))
            {
                setError("Missing CRLF after zero-size chunk", "400");
                return false;
            }
            // Read optional trailer headers until an empty line
            std::string trailer;
            while (std::getline(iss, trailer))
            {
                trimTrailingCR(trailer);
                if (trailer.empty())
                    break; // End of trailers
                // Optionally, handle trailer headers here
            }
            DEBUG_PRINT("Finished parsing chunked body, total size: " << _body.size());
            return true;
        }

        setState(PARSING_CHUNK_DATA);
        std::string chunk;
        if (!readExact(iss, chunkSize, chunk))
        {
            setError("Chunk data shorter than declared size", "400");
            return false;
        }
        _body.append(chunk);

        // Each chunk is followed by CRLF
        if (!readCRLF(iss))
        {
            setError("Missing CRLF after chunk data", "400");
            return false;
        }
    }
}

/**
 * @brief Parse body with a fixed Content-Length
 */
bool HTTPparser::parseFixedLengthBody(std::istringstream& iss, size_t length)
{
    _body.clear();
    std::string data;
    if (!readExact(iss, length, data))
    {
        setError("Body shorter than Content-Length", "400");
        return false;
    }
    _body = data;
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

