#include "Common.hpp"
#include "Cgi.hpp"

bool HttpServer::determineKeepAlive(const HTTPparser &parser)
{
    std::string conn = parser.getHeader("Connection");
    std::string version = parser.getVersion();
    // Normalize connection header to lowercase
    for (size_t i = 0; i < conn.size(); ++i)
        conn[i] = static_cast<char>(std::tolower(conn[i]));

    if (!conn.empty())
        return false; // Explicit keep-alive required for HTTP/1.0
    else if (version == "HTTP/1.1")
        return true; // Default keep-alive for HTTP/1.1

    return false;
}

// Replace `handleClient()` with Client class to implement
// non-blocking, stateful per-connection handling (read/parse/write, keep-alive).
// Integrate the Http Response logic
// Integrate the Http Request parsing logic
// HTTP 1.1 requires to support keep-alive
// Client timouts
//
void HttpServer::handleClient(int client_fd)
{
    std::string request;
    char buf[4096];

    // Read request headers (blocking)
    while (true)
    {
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
        if (n > 0)
        {
            request.append(buf, buf + n);
            size_t header_end = request.find("\r\n\r\n");
            if (header_end != std::string::npos)
            {
                // Check for Content-Length to read the body if present
                size_t contentLength = checkContentLength(request, header_end);
                if (contentLength > 0)
                {
                    // Total length = header end position + 4 (for CRLF) + body
                    size_t totalLength = header_end + 4 + contentLength;

                    // Read until we have the full body
                    while (request.size() < totalLength)
                    {
                        n = recv(client_fd, buf, sizeof(buf), 0);
                        if (n > 0)
                        {
                            request.append(buf, buf + n);
                        }
                        else if (n == 0)
                        {
                            break; // peer closed
                        }
                        else if (n < 0)
                        {
                            if (errno == EINTR)
                                continue;
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                                continue;
                            return; // fatal error
                        }
                    }
                }
                break;
            }
            continue;
        }
        if (n == 0)
            break; // peer closed
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            return; // fatal error
        }
    }

    /*HTTPparser parser;
    parser.parseRequest(request);
    bool keepAlive = determineKeepAlive(parser);

    std::string response;
    //std::string path = parser.getPath();
        //mapCurrentLocationConfig(path);
        //std::string filePath = getFilePath(path); // get the file path based on the request path and current location config

        //parser.setCurrentFilePath(filePath); // Store current file path in parser for potential CGI use
    if (!parser.isValid()) // integrate here the Http Response logic
    {
        DEBUG_PRINT(RED << "Request parsing failed: " << parser.getErrorMessage() << RESET);
        response = generateBadRequestResponse(keepAlive);
    }
    else if (parser.getMethod() == "GET") // Handle GET requests
    {
        DEBUG_PRINT("Valid request parsed:");
        DEBUG_PRINT("   Method: " << RED << parser.getMethod() << RESET);-
        DEBUG_PRINT("   Path: " << RED << parser.getPath() << RESET);
        DEBUG_PRINT("   Version: " << RED << parser.getVersion() << RESET);

        std::string path = parser.getPath();
        mapCurrentLocationConfig(path);
        std::string filePath = getFilePath(path); // get the file path based on the request path and current location config

        parser.setCurrentFilePath(filePath); // Store current file path in parser for potential CGI use

        DEBUG_PRINT(RED << "Request path: '" << path << "', mapped to file: '" << filePath << "'" << RESET);

        response = generateGetResponse(filePath, keepAlive);
    }
    else if ((parser.getMethod() == "POST") || parser.getMethod() == "GET")
    {
        std::string path = parser.getPath();
        mapCurrentLocationConfig(path);
        if (_currentLocation && _currentLocation->cgiPass && (isMethodAllowed(parser.getMethod())))
        {
            parser.setCurrentFilePath(getFilePath(parser.getPath())); // Ensure current file path is set for CGI
            response = processCGI(parser);
            if (!response.empty())
            {
                // Create the proper HTTP response based on CGI output
                response = generatePostResponse(response, keepAlive);
            }
            else
            {
                DEBUG_PRINT(RED << "CGI processing failed, sending 400 Bad Request" << RESET);
                response = generateBadRequestResponse(keepAlive);
            }
        }
        else
        {
            DEBUG_PRINT("CGI processing not enabled for this location");
            response = generateMethodNotAllowedResponse(keepAlive);
        }
    }
    else
    {
        DEBUG_PRINT("Method not allowed: " << RED << parser.getMethod() << RESET << " (only GET POST and DELETE supported)");
        response = generateMethodNotAllowedResponse(keepAlive);
    }

    // Send full response
    size_t off = 0;
    while (off < response.size())
    {
        ssize_t sent = send(client_fd, response.c_str() + off, response.size() - off, 0);
        if (sent > 0)
        {
            off += static_cast<size_t>(sent);
            continue;
        }
        if (sent < 0 && errno == EINTR)
            continue;
        if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            continue;
        break; // other error
    }*/

    // NOTE: Keep-alive not supported yet
}

// Check if the method is allowed in the current location
// Returns false if no location is set or method is not allowed
bool HttpServer::isMethodAllowed(const std::string &method)
{
    std::cout<<"Current location path: "<< (_currentLocation ? _currentLocation->path : "NULL") << std::endl;
    std::cout<<"Method: "<< method << std::endl;
    if (!_currentLocation)
        return false;

    return _currentLocation->allowedMethods.find(method) != _currentLocation->allowedMethods.end();
}

std::string HttpServer::processCGI(HTTPparser &parser, HttpServer &server)
{
    CGI cgi(parser);
    int status = cgi.execute();
    if (status != 0)
    {
        DEBUG_PRINT(RED << "CGI execution failed with status: " << status << RESET);
        return generateBadRequestResponse(determineKeepAlive(parser));
    }
    else
    {
        std::string cgiResponse = cgi.readResponse();
        cgi.cleanup();
        DEBUG_PRINT("CGI response received, length: " << cgiResponse.size());
        return cgiResponse;
    }
}

size_t HttpServer::checkContentLength(const std::string &request, size_t header_end)
{
    size_t contentLength = 0;
    size_t pos = request.find("Content-Length:", 0);
    if (pos != std::string::npos && pos < header_end)
    {
        pos += 15; // length of "Content-Length:"
        while (pos < header_end && (request[pos] == ' ' || request[pos] == '\t'))
            ++pos;
        size_t end = pos;
        while (end < header_end && std::isdigit(request[end]))
            ++end;
        if (end > pos)
        {
            std::stringstream ss(request.substr(pos, end - pos));
            ss >> contentLength;
        }
    }
    return contentLength;
}