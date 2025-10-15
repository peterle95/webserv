#include "Client.hpp"

/*
Client::readRequest()
    ↓ (reads raw bytes)
HTTPparser::parseRequest()
    ↓ (parses request line, headers, body)
Client::generateResponse()
    ↓ (uses parsed data)
HttpServer::buildXResponse()
    ↓ (generates HTTP response string)
Client::writeResponse()
    ↓ (sends to socket)
    
*/
static bool iequals(const std::string &a, const std::string &b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        char ca = static_cast<char>(std::tolower(a[i]));
        char cb = static_cast<char>(std::tolower(b[i]));
        if (ca != cb) return false;
    }
    return true;
}

Client::Client(int socket, HttpServer& server)
    : _socket(socket)
    , _server(server)
    , _state(READING)
    , _keep_alive(false)
    , _peer_half_closed(false)
    , _request_buffer()
    , _response_buffer()
    , _response_offset(0)
    , _parser()
    , _cgi_handler()
    , _cgi_pid(-1)
    , _cgi_started(false)
{
    _cgi_pipe_in[0] = _cgi_pipe_in[1] = -1;
    _cgi_pipe_out[0] = _cgi_pipe_out[1] = -1;

    DEBUG_PRINT("=== CLIENT CONSTRUCTED ===");
    DEBUG_PRINT("Socket: " << _socket);
    DEBUG_PRINT("Initial state: READING");
    DEBUG_PRINT("Setting socket to non-blocking mode");

    // Ensure socket is non-blocking
    HttpServer::setNonBlocking(_socket);
}

Client::~Client()
{
    DEBUG_PRINT(BLUE << "=== CLIENT DESTRUCTED ===" << RESET);
    DEBUG_PRINT("Socket: " << _socket);
    DEBUG_PRINT("Final state: " << (_state == CLOSING ? "CLOSING" : "UNKNOWN"));
    DEBUG_PRINT("Cleaning up CGI pipes");

    // Close CGI pipes if any
    if (_cgi_pipe_in[0] != -1) close(_cgi_pipe_in[0]);
    if (_cgi_pipe_in[1] != -1) close(_cgi_pipe_in[1]);
    if (_cgi_pipe_out[0] != -1) close(_cgi_pipe_out[0]);
    if (_cgi_pipe_out[1] != -1) close(_cgi_pipe_out[1]);
}

int Client::getSocket() const { return _socket; }
ClientState Client::getState() const { return _state; }

// main control point for client state machine
// executes logic bbased on the current state
void Client::handleConnection()
{
    DEBUG_PRINT(BLUE << "=== HANDLE CONNECTION ===" << RESET);
    DEBUG_PRINT("Current state: " << (_state == READING ? "READING" :
                                   _state == GENERATING_RESPONSE ? "GENERATING_RESPONSE" :
                                   _state == WRITING ? "WRITING" :
                                   _state == AWAITING_CGI ? "AWAITING_CGI" :
                                   _state == CLOSING ? "CLOSING" : "UNKNOWN"));

    switch (_state)
    {
        case READING:              
            readRequest(); 
            break;
        case GENERATING_RESPONSE:  
            generateResponse(); 
            break;
        case WRITING:              
            writeResponse(); 
            break;
        case AWAITING_CGI:         
            handleCgi(); 
            break;
        case CLOSING:              
            default: 
        break; // Server will close
    }
}

void Client::readRequest()
{
    DEBUG_PRINT(BLUE << "=== READING REQUEST ===" << RESET);
    DEBUG_PRINT("Buffer size before reading: " << _request_buffer.size());

    char buf[4096];
    while (true)
    {
        ssize_t n = recv(_socket, buf, sizeof(buf), 0);
        if (n > 0)
        {
            _request_buffer.append(buf, buf + n);
            DEBUG_PRINT("Received " << n << " bytes, total buffer: " << _request_buffer.size());
            // Continue loop to drain socket
            continue;
        }
        if (n == 0)
        {
            DEBUG_PRINT("Connection closed by peer");
            // Mark half-close; don't close yet — attempt to parse and respond
            _peer_half_closed = true;
            break;
        }
        if (n < 0)
        {
            if (errno == EINTR)
            {
                DEBUG_PRINT("Read interrupted, retrying...");
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                DEBUG_PRINT("No more data available (EAGAIN/EWOULDBLOCK)");
                break; // No more data for now
            }
            DEBUG_PRINT("Fatal read error: " << strerror(errno));
            // Fatal error
            _state = CLOSING;
            return;
        }
    }

    DEBUG_PRINT("Finished reading, buffer size: " << _request_buffer.size());

    // Simple heuristic: check if we have at least the header terminator
    // The HTTPparser will handle detailed validation and body completeness
    size_t header_end = _request_buffer.find("\r\n\r\n");
    if (header_end == std::string::npos)
    {
        if (_peer_half_closed && !_request_buffer.empty())
        {
            DEBUG_PRINT("Peer half-closed; attempting to parse incomplete request");
            _state = GENERATING_RESPONSE;
        }
        else
        {
            DEBUG_PRINT("Headers not complete yet, need more data");
        }
        return;
    }

    // Headers found - let HTTPparser determine if body is complete
    // For now, we'll attempt to parse. HTTPparser will validate completeness.
    DEBUG_PRINT("Headers found, transitioning to GENERATING_RESPONSE");
    _state = GENERATING_RESPONSE;
}

void Client::generateResponse()
{
    DEBUG_PRINT(BLUE << "=== GENERATING RESPONSE ===" << RESET);
    DEBUG_PRINT("Request buffer size: " << _request_buffer.size());

    // Parse the HTTP request
    DEBUG_PRINT(MAGENTA << "*** ENTERING HTTP PARSER ***" << RESET);
    bool ok = _parser.parseRequest(_request_buffer);
    _keep_alive = _server.determineKeepAlive(_parser);

    DEBUG_PRINT("Parsing result: " << (ok ? "SUCCESS" : "FAILED"));
    DEBUG_PRINT("Parser valid: " << (_parser.isValid() ? "YES" : "NO"));
    DEBUG_PRINT("Keep-alive: " << (_keep_alive ? "YES" : "NO"));

    DEBUG_PRINT(MAGENTA << "*** EXITING HTTP PARSER ***" << RESET);
    if (!ok || !_parser.isValid())
    {
        DEBUG_PRINT(RED << "Request parsing failed: " << _parser.getErrorMessage() << RESET);
        DEBUG_PRINT("Generating BAD REQUEST response");
        _response_buffer = _server.generateBadRequestResponse(false);
        _response_offset = 0;
        DEBUG_PRINT("Transitioning to WRITING state");
        _state = WRITING;
        return;
    }

    const std::string method = _parser.getMethod();
    const std::string path = _parser.getPath();

    DEBUG_PRINT("Parsed request:");
    DEBUG_PRINT("  Method: " << RED << method << RESET);
    DEBUG_PRINT("  Path: " << RED << path << RESET);
    DEBUG_PRINT("  Version: " << _parser.getVersion());

    // Map location and resolve filesystem path for this request
    std::string filePath = _server.resolveFilePathFor(path);
    DEBUG_PRINT("Resolved file path: '" << filePath << "'");

    if (iequals(method, "GET"))
    {
        DEBUG_PRINT(GREEN << "Handling GET request" << RESET);
        _response_buffer = _server.generateGetResponse(filePath, _keep_alive);
        _response_offset = 0;
        DEBUG_PRINT("GET response generated, size: " << _response_buffer.size());
        DEBUG_PRINT("Transitioning to WRITING state");
        _state = WRITING;
        return;
    }
    else if (iequals(method, "POST") || iequals(method, "DELETE"))
    {
        DEBUG_PRINT(GREEN << "Handling " << method << " request (CGI)" << RESET);
        // CGI handling requires proper location and method checks
        const LocationConfig *loc = _server.getCurrentLocation();
        bool cgiEnabled = (loc && loc->cgiPass);
        bool methodAllowed = _server.isMethodAllowed(method);

        DEBUG_PRINT(CYAN << "CGI enabled: " << (cgiEnabled ? "YES" : "NO") << RESET);
        DEBUG_PRINT(GREEN << "Method allowed: " << (methodAllowed ? "YES" : "NO") << RESET);

        if (cgiEnabled && methodAllowed)
        {
            DEBUG_PRINT(CYAN << "Processing CGI request" << RESET);
            // Provide the filesystem path for the script
            _parser.setCurrentFilePath(filePath);
            std::string cgiOut = _server.processCGI(_parser);
            DEBUG_PRINT(CYAN << "CGI output size: " << cgiOut.size() << RESET);

            // If CGI returned a full HTTP response, use it as-is; otherwise wrap as 200 OK
            if (cgiOut.compare(0, 5, "HTTP/") == 0)
            {
                DEBUG_PRINT(CYAN << "CGI returned full HTTP response" << RESET);
                _response_buffer = cgiOut;
            }
            else
            {
                DEBUG_PRINT(CYAN << "CGI returned body, wrapping as 200 OK" << RESET);
                _response_buffer = _server.generatePostResponse(cgiOut, _keep_alive);
            }

            _response_offset = 0;
            DEBUG_PRINT(CYAN << "CGI response generated, size: " << _response_buffer.size() << RESET);
            DEBUG_PRINT(YELLOW << "Transitioning to WRITING state" << RESET);
            _state = WRITING;
            return;
        }
        else
        {
            DEBUG_PRINT(CYAN << "CGI not available, generating METHOD NOT ALLOWED" << RESET);
            _response_buffer = _server.generateMethodNotAllowedResponse(_keep_alive);
            _response_offset = 0;
            DEBUG_PRINT(YELLOW << "Transitioning to WRITING state" << RESET);
            _state = WRITING;
            return;
        }
    }
    else
    {
        DEBUG_PRINT(CYAN << "Unknown method: " << RED << method << RESET);
        DEBUG_PRINT(RED << "Generating METHOD NOT ALLOWED response" << RESET);
        _response_buffer = _server.generateMethodNotAllowedResponse(_keep_alive);
        _response_offset = 0;
        DEBUG_PRINT(YELLOW << "Transitioning to WRITING state" << RESET);
        _state = WRITING;
        return;
    }
}

void Client::startCgi()
{
    DEBUG_PRINT(BLUE << "=== STARTING CGI ===" << RESET);
    DEBUG_PRINT("Transitioning from current state to AWAITING_CGI");

    // In this project, CGI is handled synchronously via HttpServer::processCGI
    // So we simply set the state to AWAITING_CGI and immediately handle it.
    _state = AWAITING_CGI;
}

void Client::handleCgi()
{
    DEBUG_PRINT(BLUE << "=== HANDLING CGI ===" << RESET);
    DEBUG_PRINT("Current state: " << (_state == AWAITING_CGI ? "AWAITING_CGI" : "UNKNOWN"));

    // Synchronous CGI handled in generateResponse(); nothing to do here for now.
    // Transition to WRITING if not already.
    if (_state == AWAITING_CGI)
    {
        DEBUG_PRINT("CGI processing complete, transitioning to WRITING");
        _state = WRITING;
    }
    else
    {
        DEBUG_PRINT("Unexpected state in handleCgi(), staying in current state");
    }
}

void Client::writeResponse()
{
    DEBUG_PRINT(BLUE << "=== WRITING RESPONSE ===" << RESET);
    DEBUG_PRINT("Response buffer size: " << _response_buffer.size());
    DEBUG_PRINT("Bytes already sent: " << _response_offset);

    while (_response_offset < _response_buffer.size())
    {
        ssize_t sent = send(_socket,
                            _response_buffer.c_str() + _response_offset,
                            _response_buffer.size() - _response_offset,
                            0);
        if (sent > 0)
        {
            _response_offset += static_cast<size_t>(sent);
            DEBUG_PRINT("Sent " << sent << " bytes, progress: " << _response_offset << "/" << _response_buffer.size());
            continue;
        }
        if (sent < 0)
        {
            if (errno == EINTR)
            {
                DEBUG_PRINT("Send interrupted, retrying...");
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                DEBUG_PRINT("Send would block, returning to try later");
                return; // try again later
            }
            DEBUG_PRINT("Fatal send error: " << strerror(errno));
            _state = CLOSING; // fatal error
            return;
        }
        // sent == 0 shouldn't happen for TCP unless closed; break
        DEBUG_PRINT("Send returned 0, connection likely closed");
        break;
    }

    DEBUG_PRINT(BLUE << "Response sending completed" << RESET);
    DEBUG_PRINT("Total bytes sent: " << _response_offset);

    if (_response_offset >= _response_buffer.size())
    {
        DEBUG_PRINT(BLUE << "Response fully sent" << RESET);

        // If the peer already half-closed its write side, close after sending
        if (_peer_half_closed)
        {
            DEBUG_PRINT("Peer half-closed earlier; transitioning to CLOSING");
            _state = CLOSING;
            return;
        }

        if (_keep_alive)
        {
            DEBUG_PRINT("Keep-alive enabled, resetting for next request");
            // Reset for next request
            _request_buffer.clear();
            _response_buffer.clear();
            _response_offset = 0;
            _parser.reset();
            _state = READING;
        }
        else
        {
            DEBUG_PRINT("Keep-alive disabled, transitioning to CLOSING");
            _state = CLOSING;
        }
    }
    else
    {
        DEBUG_PRINT(BLUE << "Response not fully sent, keeping in WRITING state" << RESET);
    }
}
