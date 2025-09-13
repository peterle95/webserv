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

void HTTPparser::parseRequestLine(std::istringstream& iss, std::string& line)
{
    // Identify which version is used 1.0 or 1.1
    // Identify method GET, POST, DELETE
    // identify path
    /*• Request Line Integrity: A server MUST NOT apply a request to the target resource 
    until it has received the entire request header section, 
    as later header fields could contain critical information like conditionals or authentication credentials*/
    // 1. Parse request line
    if (std::getline(iss, line)) {
        std::istringstream rl(line);
        rl >> _method >> _path >> _version;
        DEBUG_PRINT("Method: " << _method);
        DEBUG_PRINT("Path:   " << _path);
        DEBUG_PRINT("HTTP:   " << _version);
    }
}

void HTTPparser::parseHeaders(std::istringstream& iss, std::string& line)
{

    // Check what content is allowed
    // check if content-length is given
    // check for CRLF or \r\n at the end of each line
    // always check for key and value pairs
    // format: [field-name]:[space][field-value]
    /*• Character Restrictions: Field values MUST NOT contain carriage return (CR), line feed (LF), 
    or null (NUL) characters. If found, the recipient MUST either reject 
    the message or replace these characters with a space (SP) before further processing or forwarding. 
    Other control characters (CTL) are also invalid, but recipients MAY retain them if they appear 
    within a safe context (e.g., an application-specific quoted string). Tokens, used in field names 
    and some field values, are restricted to specific characters, excluding common delimiters.
      • Length Limitations: HTTP does not set predefined length limits for most protocol elements. 
      However, recipients SHOULD parse defensively, anticipating potentially large values. A server 
      that receives header fields, values, or sections larger than it is willing to process 
      MUST respond with an appropriate 4xx (Client Error) status code, such as 414 (URI Too Long) if the 
      URI exceeds a configured maximum length, to prevent potential attacks like request smuggling
      • URI Path Validation: When parsing the Request-URI, the server should 
      perform checks to prevent security vulnerabilities, 
      such as directory traversal (e.g., preventing access to files outside the intended 
      root directory by checking for .. in the path)
      EXTRACT THIS INFORMATION:
      
    ◦ Host: Crucial for virtual hosting, allowing a single server to host multiple domains. 
    It specifies the host and port from the target URI. For CGI, it's used to set SERVER_NAME.
    ◦ Content-Type: If the request includes a body (e.g., for POST or PUT methods), this header 
    specifies the media type of that body (e.g., text/plain, application/json, multipart/form-data), 
    which is essential for correctly parsing and processing the body content. If multipart/form-data 
    is used, the boundary for separating parts should also be extracted.
    ◦ Content-Length: For requests with a body that is not chunked, this header indicates the 
    exact size of the body in octets. This is vital for the server to know how much data to read for the body.
    ◦ Transfer-Encoding: If this header is present and its value contains "chunked", it indicates 
    that the message body is sent in a series of chunks. This requires the server to use a 
    specific parsing mechanism to reassemble the body, distinct from using Content-Length.
    ◦ Connection: This header can indicate whether the client wishes to keep the TCP connection 
    open (keep-alive) for subsequent requests or close it (close) after the current response.
    ◦ User-Agent: Provides information about the client (e.g., browser name and version, operating 
    system) for statistical purposes, debugging, or tailoring responses to specific client capabilities or limitations.
    ◦ , Accept-Encoding, Accept-Language***: These headers are used for content negotiation, 
    allowing the client to express its preferences for the media types (Accept), content encodings (Accept-Encoding), 
    and natural languages (Accept-Language) it can handle or prefers in the response. This helps the server deliver 
    the most suitable representation of the resource.
    ◦ Authorization / Proxy-Authorization: These headers carry credentials (e.g., username and password, or tokens) 
    for authenticating the user agent with the origin server or an intermediary proxy, respectively.
    ◦ Conditional Request Headers: Headers like If-Modified-Since, If-None-Match, If-Match, If-Unmodified-Since, 
    and If-Range are used by clients to make requests conditional on the state of the resource. This is primarily 
    for optimizing cache usage and preventing "lost update" problems.
    ◦ Expect: Used by clients to indicate specific expectations from the server, most notably "100-continue". 
    This allows a client to send a (potentially large) request body only after the server indicates it is ready 
    to receive it, improving efficiency.
    ◦ Referer: Indicates the URI of the resource from which the current request's URI was obtained. Useful for 
    logging, analytics, and link maintenance, but has privacy implications.
    ◦ Trailer: A response header that indicates which fields the sender anticipates sending as trailer fields 
    after the content. (While primarily a response header, its presence in a request could influence client 
    behavior or proxy forwarding of responses).
    ◦ Max-Forwards: Used with TRACE or OPTIONS methods to limit the number of times a request can be forwarded 
    by proxies, useful for debugging loops.
    ◦ DNT (Do Not Track): A non-standard, deprecated header that signaled a user's preference not to be tracked.
    ◦ Upgrade-Insecure-Requests: Indicates the client's preference for a secure response even if the request 
    was made over an insecure connection*/

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
            _request_headers[key] = val;
            DEBUG_PRINT("Header: [" << key << "] = [" << val << "]");
            if (key == "Content-Length")
                _contentLength = static_cast<size_t>(std::atoi(val.c_str()));
        }
    }
}

void HTTPparser::parseBody(std::istringstream& iss, std::string& line)
{
/*
1.Determine Body Framing from Headers:
    ◦ The Content-Length header field indicates the exact size of the entity body in octets. 
    If this header is present, the parser should read precisely that many bytes from the buffer after the 
    headers are complete. HTTP/1.0 requests with a body must include a valid Content-Length.
    ◦ The Transfer-Encoding header field, particularly with the value "chunked," specifies that the body 
    is sent in a series of chunks. This requires a different parsing mechanism.
    ◦ If neither Content-Length nor Transfer-Encoding is present, the body length might be determined by the 
    closure of the connection by the server (for responses), but this is not applicable for request bodies as 
    it prevents the server from sending a response. For requests, if Content-Length is not specified and the 
    server cannot determine the length, it should send a 400 Bad Request response.
    ◦ Methods like GET, HEAD, and DELETE generally should not have content in their requests, as this can lead 
    to security vulnerabilities like request smuggling.
2. State Machine with Buffer for Incremental Parsing:
    ◦ Since TCP is a stream-based protocol, the entire HTTP request (including the body) is not guaranteed to 
    arrive in a single recv() call.
    ◦ A state machine helps manage the parsing process as data arrives incrementally. 
    States could include: PARSING_REQUEST_LINE, PARSING_HEADERS, PARSING_BODY, PARSING_COMPLETE, and ERROR.
    ◦ A buffer (std::string _buffer or std::vector _body) is used to collect incoming data until a 
    complete section or chunk is received and processed. The HttpRequest class uses a _body vector and _body_str string for this.
3. Parsing Different Body Types:
    ◦ Fixed-Length Body: Once Content-Length is extracted from headers, the parser transitions to a 
    Message_Body state and accumulates bytes into the buffer until the specified _body_length is reached.
    ◦ Chunked Body: This involves a more complex state sequence:
        ▪ Chunked_Length_Begin and Chunked_Length: Read hexadecimal digits to determine the size of the 
        next chunk. The length is terminated by \r\n.
        ▪ Chunked_Data: Read the specified number of bytes for the chunk into the body buffer.
        ▪ Chunked_Data_CR and Chunked_Data_LF: After the data, another \r\n delimiter is expected.
        ▪ This cycle repeats until a zero-length chunk (0\r\n) is encountered, which signals the end of the body. 
        This is followed by a final \r\n (and optional trailer headers). The server needs to un-chunk these requests.
    ◦ Multipart Body: For types like multipart/form-data, the Content-Type header will include a boundary parameter. 
    The parser must identify and extract this boundary string, then use it to separate different parts within the body.
4. Important Considerations:
    ◦ Maximum Body Size: Servers typically have a configurable limit (client_max_body_size) for the request body to 
    prevent denial-of-service attacks. If the body size exceeds this limit, the server should return a 
    413 Content Too Large (Payload Too Large) status code.
    ◦ CGI Interaction: If a request with a body is destined for a CGI script, the server needs to handle the body 
    correctly (e.g., unchunking if necessary) and then feed it to the CGI script's standard input.
    ◦ Error Handling: The parser should detect invalid characters, incorrect framing, or other syntax errors, 
    setting an _error_code (e.g., 400 Bad Request, 414 URI Too Long).
    ◦ Non-blocking I/O: The state machine design is crucial because read() operations in a non-blocking server 
    might return less data than expected or immediately indicate no data available (EWOULDBLOCK or EAGAIN). 
    The parser must be able to resume from its current state when more data arrives in the buffer*/
    // for now: just read rest of iss into body
    // In a real implementation, you would handle Content-Length and Transfer-Encoding here
    std::ostringstream bodyStream;
    while (std::getline(iss, line))
    {
        bodyStream << line << "\n";
    }
    _body = bodyStream.str();

    if (!_body.empty())
        DEBUG_PRINT("Body: " << _body);
}

// Next Steps:
// 1. Connect HTTP Parser to server logic
// 2. Logic for identification which HTTP version is used (1.0 and 1.1)
// 3. Implement state machine for parsing (see enum in header file). TCP stream might come in chunks
// 4. Use buffer for collect chunks of data from HTTP requests

// Rudimentary parse
// reads the HTTP request from the socket and splits it into request line, headers and body
void HTTPparser::parseRequest(const std::string &rawRequest)
{
    // initStates();
    _HTTPrequest = rawRequest; // for debugging/logging
    std::istringstream iss(rawRequest);
    std::string line;
    _state = PARSING_REQUEST_LINE;

    if (_state == PARSING_REQUEST_LINE)
    {
        parseRequestLine(iss, line);
        _state = PARSING_HEADERS;
    }
    
    if (_state == PARSING_HEADERS)
    {
        parseHeaders(iss, line);
        _state = PARSING_BODY;
    }

    if (_state == PARSING_BODY)
    {
        parseBody(iss, line);
        _state = PARSING_COMPLETE;
    }
    if (_state == PARSING_COMPLETE)
    {
        DEBUG_PRINT("Parsing complete.");
    }
    else if (_state == ERROR)
    {
        DEBUG_PRINT("Error during parsing.");
    }
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
    return _request_headers;
}

const std::string& HTTPparser::getBody() const
{
    return _body;
}

// Setters
const std::string& HTTPparser::setMethod(const std::string &method)
{
    _method = method;
    return _method;
}
const std::string& HTTPparser::setPath(const std::string &path)
{
    _path = path;
    return _path;
}

const std::string& HTTPparser::setVersion(const std::string &version)
{
    _version = version;
    return _version;
}

const std::map<std::string, std::string>& HTTPparser::setHeaders(const std::map<std::string, std::string> &headers)
{
    _request_headers = headers;
    return _request_headers;
}

const std::string& HTTPparser::setBody(const std::string &body)
{
    _body = body;
    return _body;
}

