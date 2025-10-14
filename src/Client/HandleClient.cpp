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