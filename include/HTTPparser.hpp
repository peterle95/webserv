/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPparser.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/22 17:32:20 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/22 17:32:20 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPPARSER_HPP
#define HTTPPARSER_HPP

#include "HTTPRequestLine.hpp"
#include "HTTPHeaders.hpp"
#include <string>
#include <map>
#include <vector>

/*Since TCP is a stream-based protocol, HTTP requests are not guaranteed 
to arrive in a single packet or a single recv() call. 
A large request, a slow network, or packet fragmentation can cause the data to arrive in chunks. 
The parser must be designed to handle this incremental arrival.
The parser effectively operates as a state machine. It consumes the data in the 
buffer and transitions through several states:*/
enum State {
    PARSING_REQUEST_LINE = 0,
    PARSING_HEADERS = 1,
    PARSING_BODY = 2,
    PARSING_CHUNKED_BODY = 3,
    PARSING_CHUNK_SIZE = 4,
    PARSING_CHUNK_DATA = 5,
    PARSING_COMPLETE = 6,
    ERROR = 7
};

/*
  
  This class serves as the main controller for parsing HTTP requests.
  It uses specialized classes for parsing different parts of the request:
  - HTTPRequestLine for parsing the request line
  - HTTPHeaders for parsing headers
  - Body parsing is handled directly for now
  
  The parser operates as a state machine to handle incremental data arrival.
 */
class HTTPparser
{
    private:
        State _state;                    // Current state of the parser
        std::string _HTTPrequest;        // Raw request data for debugging
        HTTPRequestLine _requestLine;    // Request line parser
        HTTPHeaders _headers;            // Headers parser
        std::string _body;               // Body content
        std::string _buffer;             // Buffer for incremental parsing
        std::string _errorStatusCode;    // Error status if parsing fails
        bool _isValid;                   // Whether the request is valid
        std::string _errorMessage;       // Detailed error message
        
    public:
        HTTPparser();
        ~HTTPparser();

        // Main parsing methods
        bool parseRequest(const std::string &rawRequest);
        bool parseRequestLine(std::istringstream& iss);
        bool parseHeaders(std::istringstream& iss);
        bool parseBody(std::istringstream& iss);

        // State management
        State getState() const { return _state; }
        bool isValid() const { return _isValid; }
        const std::string& getErrorMessage() const { return _errorMessage; }
        const std::string& getErrorStatusCode() const { return _errorStatusCode; }
        
        // Request line accessors (delegate to HTTPRequestLine)
        const std::string& getMethod() const { return _requestLine.getMethod(); }
        const std::string& getPath() const { return _requestLine.getPath(); }
        const std::string& getVersion() const { return _requestLine.getVersion(); }
        
        // Headers accessors (delegate to HTTPHeaders)
        const std::map<std::string, std::string>& getHeaders() const { return _headers.getAllHeaders(); }
        std::string getHeader(const std::string& name) const { return _headers.getHeader(name); }
        bool hasHeader(const std::string& name) const { return _headers.hasHeader(name); }
        size_t getContentLength() const { return _headers.getContentLength(); }
        bool hasContentLength() const { return _headers.hasContentLength(); }
        bool isChunked() const { return _headers.isChunked(); }
        
        // Body accessors
        const std::string& getBody() const { return _body; }
        
        // Reset parser to initial state
        void reset();
        
};

#endif