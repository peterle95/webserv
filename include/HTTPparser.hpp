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

/*Since TCP is a stream-based protocol, HTTP requests are not guaranteed 
to arrive in a single packet or a single recv() call. 
A large request, a slow network, or packet fragmentation can cause the data to arrive in chunks. 
The parser must be designed to handle this incremental arrival.
The parser effectively operates as a state machine. It consumes the data in the 
buffer and transitions through several states:*/
enum State {
    PARSING_REQUEST_LINE = 0,
    PARSING_HEADERS = 0,
    PARSING_BODY = 0,
    PARSING_CHUNKED_BODY = 0,
    PARSING_CHUNK_SIZE = 0,
    PARSING_CHUNK_DATA = 0,
    PARSING_COMPLETE = 0,
    ERROR = 0
};

class HTTPparser
{
    private:
        State _state; // Current state of the parser
        std::string _HTTPrequest; // raw requested data, this might need to change type because large requests like POST might be too large for a string
        std::vector<std::string> _lines;
        std::string _method; // HTTP Method GET, POST, DELETE
        std::string _path; // Path of the requested resource
        std::string _version; // HTTP Version
        std::map<std::string, std::string> _request_headers; // Headers
        std::string _body; // Body
        std::string _buffer; // buffer to account for the sequential and potentially incremental nature of HTTP requests for non-blocking servers
        size_t _contentLength; // content length of the request
        std::string _errorStatusCode;         
    public:
        HTTPparser();
        ~HTTPparser();

        void parseRequest(const std::string &rawRequest);
        void parseRequestLine(std::istringstream& iss, std::string& line);
        void parseHeaders(std::istringstream& iss, std::string& line);
        void parseBody(std::istringstream& iss, std::string& line);

        // Accessors
        const std::string& getMethod() const;
        const std::string& getPath() const;
        const std::string& getVersion() const;
        const std::map<std::string, std::string>& getHeaders() const;
        const std::string& getBody() const;

        // Setters
        const std::string& setMethod(const std::string &method);
        const std::string& setPath(const std::string &path);
        const std::string& setVersion(const std::string &version);
        const std::map<std::string, std::string>& setHeaders(const std::map<std::string, std::string> &headers);
        const std::string& setBody(const std::string &body);
};

#endif