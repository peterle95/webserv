#include "Common.hpp"

// Global stop flag set by signal handlers
volatile sig_atomic_t g_stop = 0;

void handle_stop_signal(int)
{
    g_stop = 1;
}

// Helper to set a socket to non-blocking mode
void HttpServer::setupSignalHandlers()
{
    std::signal(SIGINT, handle_stop_signal);
    std::signal(SIGTERM, handle_stop_signal);
}

/*Following functions have to be integrated with http response mechanism and logging*/
void HttpServer::printStartupMessage(bool serveOnce)
{
    std::cout << "Serving " << _root << "/" << _index
              << " on http://localhost:" << _port << "/" << std::endl;

    DEBUG_PRINT(RED << "HTTP Server started on port " << _port << RESET);
    DEBUG_PRINT("Serving files from: " << RED << _root << "/" << _index << RESET);
    DEBUG_PRINT("Mode: " << (serveOnce ? RED "Single-request (CI mode)" : "Multi-request (keep-alive)") << RESET);
}

std::string HttpServer::generateBadRequestResponse(bool keepAlive)
{
    const std::string body = "<h1>400 Bad Request</h1>";
    std::ostringstream resp;

    resp << "HTTP/1.1 400 Bad Request\r\n";
    resp << "Content-Type: text/html; charset=utf-8\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    resp << "\r\n";
    resp << body;

    DEBUG_PRINT(RED << "→ Sending 400 Bad Request" << RESET);
    return resp.str();
}

// read file to string, return empty string on error. Uses binary mode to preserve file contents.
static std::string readFileToString(const std::string &path)
{
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
    if (!ifs.good())
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return std::string();
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

std::string HttpServer::generateGetResponse(const std::string &path, bool keepAlive)
{
    std::string body = readFileToString(path);
    //  std::string body = std::string("<html><body><h1>OK</h1><p>GET ") + path + "</p></body></html>";
    std::ostringstream resp;

    resp << "HTTP/1.1 200 OK\r\n";
    resp << "Content-Type: text/html; charset=utf-8\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    resp << "\r\n";
    resp << body;

    DEBUG_PRINT("→ Sending 200 OK for GET request");
    return resp.str();
}

std::string HttpServer::generateMethodNotAllowedResponse(bool keepAlive)
{
    const std::string body = "<h1>405 Method Not Allowed</h1>";
    std::ostringstream resp;

    resp << "HTTP/1.1 405 Method Not Allowed\r\n";
    resp << "Allow: GET\r\n";
    resp << "Content-Type: text/html; charset=utf-8\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    resp << "\r\n";
    resp << body;

    DEBUG_PRINT("→ Sending 405 Method Not Allowed");
    return resp.str();
}

std::string HttpServer::generatePostResponse(const std::string &body, bool keepAlive)
{
    std::ostringstream resp;
    resp << "HTTP/1.1 200 OK\r\n";
    resp << "Content-Type: text/html; charset=utf-8\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    resp << "\r\n";
    resp << body;

    DEBUG_PRINT("→ Sending 200 OK for POST request");
    return resp.str();
}