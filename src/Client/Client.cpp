#include "Client.hpp"
#include "Logger.hpp"
#include <ctime> // NEW

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
/*static bool iequals(const std::string &a, const std::string &b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        char ca = static_cast<char>(std::tolower(a[i]));
        char cb = static_cast<char>(std::tolower(b[i]));
        if (ca != cb)
            return false;
    }
    return true;
}*/

Client::Client(int fd, HttpServer &server, size_t serverIndex, int serverPort)
    : _socket(fd),

      _server(server),
      _response(NULL),
      _state(READING),
      _keep_alive(false),
      _peer_half_closed(false),
      _request_buffer(),
      _response_buffer(),
      _response_offset(0),
      _parser(),
      _cgi_handler(),
      _cgi_pid(-1),
      _cgi_started(false),
      _cgi_start_time(0), 
      _last_activity_time(time(NULL)), // Initialize with current time
      _serverIndex(serverIndex),
      _serverPort(serverPort),
      _status_code(200)

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
    if (_cgi_pipe_in[0] != -1)
        close(_cgi_pipe_in[0]);
    if (_cgi_pipe_in[1] != -1)
        close(_cgi_pipe_in[1]);
    if (_cgi_pipe_out[0] != -1)
        close(_cgi_pipe_out[0]);
    if (_cgi_pipe_out[1] != -1)
        close(_cgi_pipe_out[1]);
    // Delete response object if created
    if (_response != NULL)
    {
        delete _response;
        _response = NULL;
    }
}

int Client::getSocket() const { return _socket; }
ClientState Client::getState() const { return _state; }

// main control point for client state machine
// executes logic bbased on the current state
void Client::handleConnection()
{
    DEBUG_PRINT(BLUE << "=== HANDLE CONNECTION ===" << RESET);
    DEBUG_PRINT("Current state: " << (_state == READING ? "READING" : _state == GENERATING_RESPONSE ? "GENERATING_RESPONSE"
                                                                  : _state == WRITING               ? "WRITING"
                                                                  : _state == CLOSING               ? "CLOSING"
                                                                  : _state == CGI_READING_OUTPUT    ? "CGI_READING_OUTPUT"
                                                                  : _state == CGI_WRITING_INPUT     ? "CGI_WRITING_INPUT"
                                                                                                    : "UNKNOWN"));
    checkCgiTimeout();
    switch (_state)
    {
    case READING:
        readRequest();
        break;
    case GENERATING_RESPONSE:
        generateResponse();
        break;
    case CGI_WRITING_INPUT:
        writeToCgi();
        break;
    case CGI_READING_OUTPUT:
        readFromCgi();
        break;
    case WRITING:
        writeResponse();
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
    ssize_t n = recv(_socket, buf, sizeof(buf), 0);
    if (n > 0)
    {
        updateLastActivityTime(); // Update activity on successful read
        _request_buffer.append(buf, buf + n);
        DEBUG_PRINT("Received " << n << " bytes, total buffer: " << _request_buffer.size());
        size_t header_end = _request_buffer.find("\r\n\r\n");
        DEBUG_PRINT(CYAN << "Current header end position: " << header_end << RESET);
        if (header_end != std::string::npos)
        {
            // Check for Content-Length to read the body if present
            size_t contentLength = checkContentLength(_request_buffer, header_end);
            DEBUG_PRINT(CYAN << "Detected Content-Length: " << contentLength << RESET);
            bool isChunked = hasChunked(_request_buffer, header_end);
            if (contentLength > 0 || isChunked)
            {
                size_t maxBodySize = _server.getServerMaxBodySize(_serverIndex);
                if (maxBodySize != 0 && contentLength > maxBodySize)
                {

                    DEBUG_PRINT(RED << "Warning:Request body exceeds maximum allowed size" << RESET);
                    // Set status code, but continue reading to drain the socket
                    _status_code = 413;
                }
                // Total length = header end position + 4 (for CRLF) + body
                // size_t totalLength = header_end + 4 + contentLength;

                // Check how many body bytes we have already
                size_t have = 0;
                if (_request_buffer.size() > header_end + 4)
                    have = _request_buffer.size() - (header_end + 4);

                if ((!isChunked && have < contentLength) || (isChunked && checkEnd(_request_buffer, header_end) == false))
                {
                    DEBUG_PRINT("Incomplete body: have " << have << " of " << contentLength << " bytes");
                    if (_peer_half_closed)
                    {
                        DEBUG_PRINT("Peer half-closed, will attempt to parse incomplete request");
                        // fall through to parse attempt
                    }
                    else
                    {
                        DEBUG_PRINT("Need more body data; staying in READING state");
                        return; // wait for next event (socket becomes readable)
                    }
                }
            }
        }
        else
        {
            // Headers incomplete, need more data
            return;
        }
    }
    if (n == 0)
    {
        DEBUG_PRINT("Connection closed by peer");

        // Check if we have valid HTTP headers before attempting to parse
        size_t header_end = _request_buffer.find("\r\n\r\n");
        if (header_end != std::string::npos)
        {
            // We have complete headers, mark half-close and try to parse
            DEBUG_PRINT("Peer half-closed after sending headers; will attempt to parse");
            _peer_half_closed = true;
        }
        else if (!_request_buffer.empty())
        {
            // Peer closed without sending complete headers
            // This is likely TLS/SSL data or invalid HTTP - close immediately
            DEBUG_PRINT("Peer closed without sending complete HTTP headers; closing connection");
            _state = CLOSING;
            return;
        }
        else
        {
            // Empty buffer and peer closed - just close
            DEBUG_PRINT("Peer closed with no data; closing connection");
            _state = CLOSING;
            return;
        }
    }
    else if (n < 0)
    {
        DEBUG_PRINT("Fatal read error: " << strerror(errno));
        // Fatal error
        _state = CLOSING;
        return;
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
            Logger::logRequest(_request_buffer);
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
    Logger::logRequest(_request_buffer);
}

void Client::generateResponse()
{
    bool ok = false;
    DEBUG_PRINT(BLUE << "=== GENERATING RESPONSE ===" << RESET);
    DEBUG_PRINT("Request buffer size: " << _request_buffer.size());

    // Parse the HTTP request
    DEBUG_PRINT(MAGENTA << "*** ENTERING HTTP PARSER ***" << RESET);
    if (_status_code != 200)
    {
        DEBUG_PRINT(RED << "Skipping parsing due to pre-set error code: " << _status_code << RESET);
        _keep_alive = false; // Always close on bad request
    }
    else
    {
        ok = _parser.parseRequest(_request_buffer);
        _keep_alive = _server.determineKeepAlive(_parser);
    }

    DEBUG_PRINT("Parsing result: " << (ok ? "SUCCESS" : "FAILED"));
    DEBUG_PRINT("Parser valid: " << (_parser.isValid() ? "YES" : "NO"));
    DEBUG_PRINT("Keep-alive: " << (_keep_alive ? "YES" : "NO"));

    DEBUG_PRINT(MAGENTA << "*** EXITING HTTP PARSER ***" << RESET);
    if (_response != NULL)
    {
        delete _response;
        _response = NULL;
    }
    // Response object must be created regardless of whether parsing is successful or not, to handle error responses
    _response = new Response(_server, _parser, _server._configParser);
        // Select correct server based on Host header
        size_t selectedServerIndex = _server.selectServerForRequest(_parser, _serverPort);
        if (_response)
            _response->setServerIndex(selectedServerIndex);

    if (ok && _parser.isValid())
    {
            DEBUG_PRINT(GREEN << "Request parsed successfully" << RESET);
        if (selectedServerIndex == static_cast<size_t>(-1))
        {
            DEBUG_PRINT(RED << "No matching server block for Host/Port" << RESET);
            _status_code = 400;
            if (_response)
                _response_buffer = _response->processResponse(_parser.getMethod(), _status_code, "");
            Logger::logResponse(_response_buffer);
            _response_offset = 0;
            DEBUG_PRINT("Transitioning to WRITING state");
            _state = WRITING;
            return;
        }

        DEBUG_PRINT(CYAN << "Selected server index: " << selectedServerIndex
                         << " for Host: '" << _parser.getHeader("Host") << "'" << RESET);
        const std::string path = _parser.getPath();

        // Map location and resolve filesystem path for this request
        std::string filePath = _server.resolveFilePathFor(path, selectedServerIndex);
        _parser.setCurrentFilePath(filePath);
        DEBUG_PRINT("Resolved file path: '" << filePath << "'");

        // CGI handling requires proper location and method checks
        const LocationConfig *loc = _server.getCurrentLocation();
        bool cgiEnabled = (loc && loc->cgiPass);
        std::string method = _parser.getMethod();
        bool methodAllowed = _server.isMethodAllowed(method);

        if (methodAllowed && (method == "GET" || method == "POST" || method == "DELETE") && cgiEnabled)
        {
            // Check if file extension matches CGI extension
            bool isCgiScript = false;
            if (!loc->cgiExtension.empty())
            {
                size_t ext_pos = filePath.rfind('.');
                if (ext_pos != std::string::npos)
                {
                    std::string ext = filePath.substr(ext_pos);
                    isCgiScript = (ext == loc->cgiExtension);
                }
            }
            if (isCgiScript)
            {
                DEBUG_PRINT(CYAN << "Starting non-blocking CGI" << RESET);

                // === If peer half-closed and body incomplete, DO NOT start CGI ===
                std::string cl_hdr = _parser.getHeader("Content-Length");
                if (_peer_half_closed && !cl_hdr.empty())
                {
                    size_t expected = 0;
                    std::istringstream iss(cl_hdr);
                    iss >> expected;
                    if (_parser.getBody().size() < expected)
                    {
                        DEBUG_PRINT(RED << "Peer half-closed with incomplete body; aborting CGI" << RESET);
                        _status_code = 400;
                        if (_response)
                            _response_buffer = _response->processResponse(_parser.getMethod(), _status_code, "");
                        Logger::logResponse(_response_buffer);
                        _response_offset = 0;
                        DEBUG_PRINT("Transitioning to WRITING state");
                        _state = WRITING;
                        return;
                    }
                }

                // Create CGI handler
                _cgi_handler = CGI(_parser);

                // Start CGI process (non-blocking)
                if (_cgi_handler.execute() == 0)
                {
                    _cgi_pid = _cgi_handler.getPid();
                    _cgi_pipe_in[1] = _cgi_handler.getInputFd();
                    _cgi_pipe_out[0] = _cgi_handler.getOutputFd();
                    _cgi_input_offset = 0;
                    _cgi_output_buffer.clear();
                    _cgi_started = true;
                    _cgi_start_time = time(NULL); // NEW

                    // Transition to writing input state
                    _state = CGI_WRITING_INPUT;
                    DEBUG_PRINT("CGI forked successfully, PID: " << _cgi_pid);
                }
                else
                {
                    DEBUG_PRINT(RED << "CGI fork failed" << RESET);
                    _status_code = 500;
                }
                return;
            }
            else
            {
                _status_code = 404;
                DEBUG_PRINT(RED << "CGI script not found or extension mismatch" << RESET);
            }
        }
    }
    // If we reach here, either parsing failed, error occurred or we are not doing CGI
    // If parsing failed, parser would have set error status code which will be checked inside processResponse
    if (_response)
        _response_buffer = _response->processResponse(_parser.getMethod(), _status_code, "");
    Logger::logResponse(_response_buffer);
    _response_offset = 0;
    DEBUG_PRINT("Transitioning to WRITING state");
    _state = WRITING;
    return;
}

void Client::writeResponse()
{
    DEBUG_PRINT(BLUE << "=== WRITING RESPONSE ===" << RESET);
    DEBUG_PRINT("Response buffer size: " << _response_buffer.size());
    DEBUG_PRINT("Bytes already sent: " << _response_offset);

    // Check if response is already complete BEFORE attempting send
    if (_response_offset >= _response_buffer.size())
    {
        DEBUG_PRINT(BLUE << "Response fully sent, handling completion" << RESET);

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
        return;
    }

    ssize_t sent = send(_socket,
                        _response_buffer.c_str() + _response_offset,
                        _response_buffer.size() - _response_offset,
                        0);

    if (sent > 0)
    {
        updateLastActivityTime(); // Update activity on successful write
        _response_offset += static_cast<size_t>(sent);
        DEBUG_PRINT("Sent " << sent << " bytes, progress: " << _response_offset << "/" << _response_buffer.size());

        // Check if we just completed the response
        if (_response_offset >= _response_buffer.size())
        {
            DEBUG_PRINT(BLUE << "Response sending completed" << RESET);
            DEBUG_PRINT("Total bytes sent: " << _response_offset);

            if (_peer_half_closed)
            {
                DEBUG_PRINT("Peer half-closed earlier; transitioning to CLOSING");
                _state = CLOSING;
                return;
            }

            if (_keep_alive)
            {
                DEBUG_PRINT("Keep-alive enabled, resetting for next request");
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
    else if (sent == 0)
    {
        DEBUG_PRINT("Send returned 0, connection likely closed");
        _state = CLOSING;
    }
    else // sent < 0
    {
        DEBUG_PRINT("Send failed, transitioning to CLOSING");
        _state = CLOSING;
    }
}

size_t Client::checkContentLength(const std::string &request, size_t header_end)
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

// Return true if the header block contains Transfer-Encoding: chunked (case-insensitive)
// This is a helper for determining if we need to look for chunked body while reading
bool Client::hasChunked(const std::string &request, size_t header_end) const
{
    size_t pos = request.find("Transfer-Encoding:", 0);
    if (pos == std::string::npos || pos >= header_end)
        return false;
    pos += strlen("Transfer-Encoding:");
    while (pos < header_end && (request[pos] == ' ' || request[pos] == '\t'))
        ++pos;
    size_t end = pos;
    while (end < header_end && request[end] != '\r' && request[end] != '\n')
        ++end;
    std::string val = request.substr(pos, end - pos);
    // lowercase check for "chunked"
    for (size_t i = 0; i < val.size(); ++i)
        val[i] = static_cast<char>(::tolower(val[i]));
    return (val.find("chunked") != std::string::npos);
}

// Quick heuristic: look for common final-zero-chunk patterns.
// Returns true if it very likely contains the terminating 0-chunk (+ optional trailers).
bool Client::checkEnd(const std::string &request, size_t header_end) const
{
    if (header_end == std::string::npos)
        return false;
    size_t body_start = header_end + 4; // after "\r\n\r\n"
    if (body_start >= request.size())
        return false;

    // common full terminator: CRLF "0" CRLF CRLF
    if (request.find("\r\n0\r\n\r\n", body_start) != std::string::npos)
        return true;

    // common terminator without trailing blank line (some clients)
    if (request.find("\r\n0\r\n", body_start) != std::string::npos)
        return true;

    // zero chunk may be at body_start (no preceding CRLF)
    if (request.compare(body_start, 4, "0\r\n\r\n") == 0 || request.compare(body_start, 3, "0\r\n") == 0)
        return true;

    // chunk extensions: look for "\r\n0;" then CRLF and final CRLFCRLF afterwards
    size_t p = request.find("\r\n0;", body_start);
    if (p != std::string::npos)
    {
        size_t afterSizeLine = request.find("\r\n", p + 3);
        if (afterSizeLine != std::string::npos)
        {
            if (request.find("\r\n\r\n", afterSizeLine + 2) != std::string::npos)
                return true;
        }
    }

    return false;
}

// Write request body to CGI incrementally

void Client::writeToCgi()
{
    DEBUG_PRINT(BLUE << "=== WRITING TO CGI ===" << RESET);

    const std::string &body = _parser.getBody();
    size_t body_size = body.size();

    // Check if all data has been written
    if (_cgi_input_offset >= body_size)
    {
        DEBUG_PRINT(GREEN << "Finished writing request body to CGI" << RESET);
        close(_cgi_pipe_in[1]);
        _cgi_pipe_in[1] = -1;
        _state = CGI_READING_OUTPUT;
        return;
    }

    // Attempt ONE write operation
    ssize_t written = write(_cgi_pipe_in[1],
                            body.c_str() + _cgi_input_offset,
                            body_size - _cgi_input_offset);

    if (written > 0)
    {
        // Successfully wrote some bytes
        _cgi_input_offset += static_cast<size_t>(written);
        DEBUG_PRINT("Wrote " << written << " bytes to CGI, offset: "
                             << _cgi_input_offset << "/" << body_size);

        // Check if we've now written everything
        if (_cgi_input_offset >= body_size)
        {
            DEBUG_PRINT(GREEN << "Finished writing request body to CGI" << RESET);
            close(_cgi_pipe_in[1]);
            _cgi_pipe_in[1] = -1;
            _state = CGI_READING_OUTPUT;
        }
        // Otherwise, stay in CGI_WRITING_INPUT and return to select()
        return;
    }
    else if (written == 0)
    {
        // Unusual for write() but possible - treat as non-fatal
        DEBUG_PRINT("CGI input pipe returned 0, waiting for next select() wake");
        return; // Stay in CGI_WRITING_INPUT, select() will wake us when ready
    }
    else // written == -1
    {
        // Write failed - could be EAGAIN (pipe full) or real error
        // Without checking errno, treat ALL errors as fatal
        DEBUG_PRINT(RED << "Error writing to CGI, aborting" << RESET);
        _status_code = 400;
        if (_response)
            _response_buffer = _response->processResponse(_parser.getMethod(), _status_code, "");
        Logger::logResponse(_response_buffer);
        _response_offset = 0;
        DEBUG_PRINT("Transitioning to WRITING state");
        _state = WRITING;
        cleanup_cgi();
        return;
    }

    // Finished writing input to CGI
    DEBUG_PRINT(GREEN << "Finished writing request body to CGI" << RESET);
    close(_cgi_pipe_in[1]);
    _cgi_pipe_in[1] = -1;

    // Transition to reading output from CGI
    _state = CGI_READING_OUTPUT;
}

// Read CGI output incrementally
void Client::readFromCgi()
{
    DEBUG_PRINT(BLUE << "=== READING FROM CGI ===" << RESET);

    char buf[4096];
    ssize_t n = read(_cgi_pipe_out[0], buf, sizeof(buf));
    if (n > 0)
    {
        _cgi_output_buffer.append(buf, buf + n);
        DEBUG_PRINT("Read " << n << " bytes from CGI, total output size: " << _cgi_output_buffer.size());
        return; // Stay in CGI_READING_OUTPUT, select() will wake us when more data is available
    }
    else if (n == 0)
    {
        // EOF - CGI finished writing
        DEBUG_PRINT(GREEN << "CGI output pipe closed, finished reading" << RESET);
        close(_cgi_pipe_out[0]);
        _cgi_pipe_out[0] = -1;

        int status;
        pid_t result = waitpid(_cgi_pid, &status, WNOHANG);

        // If child exited successfully
        if (result == _cgi_pid)
        {
            // Child exited
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            {
                DEBUG_PRINT(GREEN << "CGI completed successfully" << RESET);
            }
            else
            {
                DEBUG_PRINT(RED << "CGI failed with exit status " << status << RESET);
                _status_code = 500;
                _keep_alive = false;
            }
        }
        else if (result == 0)
        {
            // Child still running but pipe closed - build response anyway
            DEBUG_PRINT("CGI pipe closed but process still running");

            if (_cgi_output_buffer.empty())
            {
                _status_code = 400;
            }
        }
        else // result == -1
        {
            // waitpid error
            DEBUG_PRINT(RED << "waitpid failed" << RESET);
            _status_code = 400;
        }
    }
    else // n == -1
    {
        // Read error - abort CGI and generate error response
        DEBUG_PRINT(RED << "Error reading from CGI, aborting" << RESET);
        _status_code = 400;
    }
    if (_response)
    {
        _response_buffer = _response->processResponse(_parser.getMethod(), _status_code, _cgi_output_buffer);
    }
    Logger::logResponse(_response_buffer);
    _response_offset = 0;
    DEBUG_PRINT("Transitioning to WRITING state");
    _state = WRITING;
    cleanup_cgi();
}

void Client::cleanup_cgi()
{
    DEBUG_PRINT(BLUE << "=== CLEANING UP CGI ===" << RESET);

    if (_cgi_pipe_in[1] != -1)
    {
        close(_cgi_pipe_in[1]);
        _cgi_pipe_in[1] = -1;
    }
    if (_cgi_pipe_out[0] != -1)
    {
        close(_cgi_pipe_out[0]);
        _cgi_pipe_out[0] = -1;
    }
    if (_cgi_pid != -1)
    {
        kill(_cgi_pid, SIGKILL);
        waitpid(_cgi_pid, NULL, WNOHANG);
        _cgi_pid = -1;
    }
}

void Client::checkCgiTimeout() // NEW
{
    // Guard clause: Return immediately if no CGI process is running
    if (_cgi_pid == -1 || (_state != CGI_WRITING_INPUT && _state != CGI_READING_OUTPUT))
        return;

    DEBUG_PRINT("Checking CGI timeout - elapsed: " << difftime(time(NULL), _cgi_start_time) << "s / " << CGI_TIMEOUT << "s"); // NEW DEBUG

    // Check timeout
    if (difftime(time(NULL), _cgi_start_time) > CGI_TIMEOUT)
    {
        DEBUG_PRINT(RED << "CGI timed out after " << CGI_TIMEOUT << " seconds" << RESET);
        cleanup_cgi();
        _status_code = 504; // Gateway Timeout
        
        if (_response)
            _response_buffer = _response->processResponse(_parser.getMethod(), _status_code, "");
            
        Logger::logResponse(_response_buffer);
        _response_offset = 0;
        _state = WRITING;
    }
}

void Client::updateLastActivityTime()
{
    _last_activity_time = time(NULL);
}

bool Client::hasTimedOut() const
{
    return (difftime(time(NULL), _last_activity_time) > CLIENT_TIMEOUT);
}