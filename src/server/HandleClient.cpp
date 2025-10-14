#include "Common.hpp"
#include "Cgi.hpp"

bool HttpServer::determineKeepAlive(const HTTPparser &parser)
{
    std::string conn = parser.getHeader("Connection");
    std::string version = parser.getVersion();
    // Normalize connection header to lowercase
    for (size_t i = 0; i < conn.size(); ++i)
        conn[i] = static_cast<char>(std::tolower(conn[i]));

    DEBUG_PRINT("Determining keep-alive: Connection='" << conn << "', Version='" << version << "'");

    if (conn.empty())
    {
        DEBUG_PRINT("Keep-alive: NO (HTTP/1.0 without explicit Connection header)");
        return false; // Explicit keep-alive required for HTTP/1.0
    }
    else if (version == "HTTP/1.1")
    {
        DEBUG_PRINT("Keep-alive: YES (HTTP/1.1 default)");
        return true; // Default keep-alive for HTTP/1.1
    }

    DEBUG_PRINT("Keep-alive: NO (HTTP/1.0 with Connection header but not 1.1)");
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
    DEBUG_PRINT("=== NEW CLIENT CONNECTION ===");
    DEBUG_PRINT("Client socket: " << client_fd);

    std::string request;
    char buf[4096];

    // Read request headers (blocking)
    while (true)
    {
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
        if (n > 0)
        {
            request.append(buf, buf + n);
            DEBUG_PRINT("Received " << n << " bytes, total request size: " << request.size());

            size_t header_end = request.find("\r\n\r\n");
            if (header_end != std::string::npos)
            {
                DEBUG_PRINT("Headers received, length: " << header_end + 4);
                // Check for Content-Length to read the body if present
                size_t contentLength = checkContentLength(request, header_end);
                if (contentLength > 0)
                {
                    DEBUG_PRINT("Content-Length found: " << contentLength << " bytes");
                    // Total length = header end position + 4 (for CRLF) + body
                    size_t totalLength = header_end + 4 + contentLength;

                    // Read until we have the full body
                    while (request.size() < totalLength)
                    {
                        n = recv(client_fd, buf, sizeof(buf), 0);
                        if (n > 0)
                        {
                            request.append(buf, buf + n);
                            DEBUG_PRINT("Body bytes received: " << n << ", progress: " << request.size() << "/" << totalLength);
                        }
                        else if (n == 0)
                        {
                            DEBUG_PRINT("Connection closed while reading body");
                            break; // peer closed
                        }
                        else if (n < 0)
                        {
                            if (errno == EINTR)
                                continue;
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                                continue;
                            DEBUG_PRINT("Error reading body: " << strerror(errno));
                            return; // fatal error
                        }
                    }
                    DEBUG_PRINT("Body reading completed, total request size: " << request.size());
                }
                else
                {
                    DEBUG_PRINT("No Content-Length header found, request complete");
                }
                break;
            }
            continue;
        }
        if (n == 0)
        {
            DEBUG_PRINT("Connection closed by peer during request reading");
            break; // peer closed
        }
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            DEBUG_PRINT("Fatal error reading request: " << strerror(errno));
            return; // fatal error
        }
    }

    DEBUG_PRINT("=== PARSING REQUEST ===");
    DEBUG_PRINT("Request size: " << request.size() << " bytes");

    HTTPparser parser;
    parser.parseRequest(request);
    bool keepAlive = determineKeepAlive(parser);

    DEBUG_PRINT("Parsing completed. Keep-Alive: " << (keepAlive ? "YES" : "NO"));
    DEBUG_PRINT("Connection header: '" << parser.getHeader("Connection") << "'");
    DEBUG_PRINT("HTTP version: " << parser.getVersion());

    std::string response;
    if (!parser.isValid()) // integrate here the Http Response logic
    {
        DEBUG_PRINT(RED << "Request parsing failed: " << parser.getErrorMessage() << RESET);
        DEBUG_PRINT("Generating BAD REQUEST response");
        response = generateBadRequestResponse(keepAlive);
    }
    else if (parser.getMethod() == "GET")
    {
        DEBUG_PRINT("Valid request parsed:");
        DEBUG_PRINT("   Method: " << RED << parser.getMethod() << RESET);
        DEBUG_PRINT("   Path: " << RED << parser.getPath() << RESET);
        DEBUG_PRINT("   Version: " << RED << parser.getVersion() << RESET);

        std::string path = parser.getPath();
        mapCurrentLocationConfig(path);
        std::string filePath = getFilePath(path); // get the file path based on the request path and current location config

        parser.setCurrentFilePath(filePath); // Store current file path in parser for potential CGI use

        DEBUG_PRINT(RED << "Request path: '" << path << "', mapped to file: '" << filePath << "'" << RESET);
        DEBUG_PRINT("Generating GET response");

        response = generateGetResponse(filePath, keepAlive);
    }
    else if ((parser.getMethod() == "POST") || (parser.getMethod() == "DELETE"))
    {
        DEBUG_PRINT("CGI request: Method=" << parser.getMethod() << ", Path=" << parser.getPath());
        if (_currentLocation && _currentLocation->cgiPass && (isMethodAllowed(parser.getMethod())))
        {
            parser.setCurrentFilePath(getFilePath(parser.getPath())); // Ensure current file path is set for CGI
            DEBUG_PRINT("CGI enabled for this location, processing CGI");
            response = processCGI(parser);
            if (!response.empty())
            {
                // Create the proper HTTP response based on CGI output
                DEBUG_PRINT("CGI processing successful, wrapping response");
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

    DEBUG_PRINT("=== RESPONSE READY ===");
    DEBUG_PRINT("Response size: " << response.size() << " bytes");
    DEBUG_PRINT("Response preview: " << response.substr(0, std::min<size_t>(100, response.size())) << (response.size() > 100 ? "..." : ""));

    // Send full response
    size_t off = 0;
    DEBUG_PRINT("=== SENDING RESPONSE ===");
    while (off < response.size())
    {
        ssize_t sent = send(client_fd, response.c_str() + off, response.size() - off, 0);
        if (sent > 0)
        {
            off += static_cast<size_t>(sent);
            DEBUG_PRINT("Sent " << sent << " bytes, progress: " << off << "/" << response.size());
            continue;
        }
        if (sent < 0 && errno == EINTR)
        {
            DEBUG_PRINT("Send interrupted, retrying...");
            continue;
        }
        if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            DEBUG_PRINT("Send would block, retrying...");
            continue;
        }
        DEBUG_PRINT("Fatal send error: " << strerror(errno));
        break; // other error
    }

    DEBUG_PRINT("Response sent successfully, total bytes: " << off);

    DEBUG_PRINT("=== CLIENT CONNECTION HANDLED ===");
    DEBUG_PRINT("Keep-alive support: NOT IMPLEMENTED (connection will close)");
    DEBUG_PRINT("=================================");
}

// Check if the method is allowed in the current location
// Returns false if no location is set or method is not allowed
bool HttpServer::isMethodAllowed(const std::string &method)
{
    if (!_currentLocation)
        return false;

    return _currentLocation->allowedMethods.find(method) != _currentLocation->allowedMethods.end();
}

std::string HttpServer::processCGI(HTTPparser &parser)
{
    CGI cgi(parser, *this);
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