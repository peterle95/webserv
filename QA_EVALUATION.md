# Webserv Evaluation Q&A

This document contains common questions evaluators might ask during the evaluation, with detailed answers and code references.

## Table of Contents
- [HTTP Server Basics](#http-server-basics)
- [I/O Multiplexing](#io-multiplexing)
- [select() Implementation](#select-implementation)
- [Error Handling](#error-handling)
- [Non-Blocking I/O](#non-blocking-io)
- [Client State Machine](#client-state-machine)
- [CGI Implementation](#cgi-implementation)
- [Configuration](#configuration)
- [Request Parsing](#request-parsing)
- [Response Generation](#response-generation)
- [Advanced Topics](#advanced-topics)

---

## HTTP Server Basics

### Q1: What is an HTTP server and how does it work?

**Answer:**
An HTTP server is software that listens for HTTP requests from clients and sends back HTTP responses.

**Basic workflow:**
1. **Bind to a port** - Create socket and bind to port (e.g., 8080)
2. **Listen** - Start listening for incoming connections
3. **Accept** - Accept client connections
4. **Read request** - Read HTTP request from client
5. **Process** - Parse request and determine action
6. **Generate response** - Create HTTP response
7. **Send response** - Send response back to client
8. **Close or keep-alive** - Close connection or keep it open

**Code Reference:**
```cpp
// File: src/server/HttpServer.cpp, lines 125-173
int HttpServer::start()
{
    for (size_t serverIdx = 0; serverIdx < _servers.size(); ++serverIdx)
    {
        const ServerConfig &serverConfig = _servers[serverIdx];
        const std::vector<int> &ports = serverConfig.getListenPorts();

        for (size_t portIdx = 0; portIdx < ports.size(); ++portIdx)
        {
            int port = ports[portIdx];
            int server_fd = createAndBindSocket(port);
            if (server_fd < 0)
                continue;

            if (!setNonBlocking(server_fd))
            {
                close(server_fd);
                continue;
            }

            _serverSockets.push_back(ServerSocketInfo(server_fd, port, serverIdx));
        }
    }
    
    return runMultiServerAcceptLoop(_serverSockets, serveOnce);
}
```

---

## I/O Multiplexing

### Q2: What I/O multiplexing function did you use?

**Answer:** We used `select()` for I/O multiplexing.

**Code Reference:**
```cpp
// File: src/server/AcceptSocket.cpp, line 52
int ready = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
```

### Q3: What is I/O multiplexing and why do we need it?

**Answer:**
I/O multiplexing allows monitoring multiple file descriptors simultaneously to see which are ready for I/O operations.

**Why we need it:**
- **Efficiency**: No need for one thread per connection
- **Scalability**: Handle thousands of connections with one thread
- **Responsiveness**: Don't block on slow clients

**Alternatives:**
- `poll()` - Similar but no FD_SETSIZE limit
- `epoll()` (Linux) - More efficient for many FDs
- `kqueue()` (BSD/macOS) - Similar to epoll
- Multi-threading - One thread per connection (resource intensive)
- Blocking I/O - One connection at a time (very inefficient)

---

## select() Implementation

### Q4: How does select() work?

**Answer:**
`select()` monitors multiple file descriptors and returns when one or more become ready for I/O.

**Function signature:**
```c
int select(int nfds, fd_set *readfds, fd_set *writefds, 
           fd_set *exceptfds, struct timeval *timeout);
```

**How it works:**
1. Create fd_sets and add FDs to monitor
2. Call select() - kernel checks all FDs
3. Process sleeps if none ready
4. Wakes when FD ready, timeout expires, or signal received
5. Returns number of ready FDs
6. Check specific FDs with FD_ISSET()

**Our Implementation:**
```cpp
// File: src/server/AcceptSocket.cpp, lines 18-77
int HttpServer::runMultiServerAcceptLoop(...)
{
    while (!g_stop)
    {
        fd_set read_fds, write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        int max_fd = -1;

        // Add server sockets to read set
        for (size_t i = 0; i < serverSockets.size(); ++i)
        {
            int server_fd = serverSockets[i].socket_fd;
            FD_SET(server_fd, &read_fds);
            if (server_fd > max_fd)
                max_fd = server_fd;
        }

        // Add client sockets based on state
        for (std::map<int, Client *>::iterator it = _clients.begin(); 
             it != _clients.end(); ++it)
        {
            int cfd = it->first;
            Client *cl = it->second;
            ClientState st = cl->getState();
            
            if (st == READING)
            {
                FD_SET(cfd, &read_fds);
                if (cfd > max_fd) max_fd = cfd;
            }
            else if (st == WRITING)
            {
                FD_SET(cfd, &write_fds);
                if (cfd > max_fd) max_fd = cfd;
            }
        }

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ready = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
        
        if (ready < 0)
        {
            if (errno == EINTR) continue;
            std::cerr << "select() failed" << std::endl;
            break;
        }

        // Process ready FDs...
    }
}
```

### Q5: Do you use only ONE select()? How does it handle accepting and client I/O?

**Answer:** YES, we use **ONLY ONE** `select()` in the main loop. It monitors:
- Server listening sockets (for accepting)
- Client sockets for reading
- Client sockets for writing

**Code showing single select():**
```cpp
// File: src/server/AcceptSocket.cpp, line 52
// This is the ONLY select() call
int ready = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
```

**How we manage both:**
```cpp
// Server sockets always in read_fds (for accepting)
FD_SET(server_fd, &read_fds);

// Clients added based on state
if (st == READING)
    FD_SET(cfd, &read_fds);    // Read from client
else if (st == WRITING)
    FD_SET(cfd, &write_fds);   // Write to client

// After select(), check all
if (FD_ISSET(server_fd, &read_fds))
    accept();  // New connection
    
if (FD_ISSET(cfd, &read_fds))
    readRequest();  // Read from client
    
if (FD_ISSET(cfd, &write_fds))
    writeResponse();  // Write to client
```

The `select()` checks read_fds AND write_fds **simultaneously**.

---

## Error Handling

### Q6: How do you handle recv/send errors?

**Answer:** We handle all return values: positive (success), 0 (closed), negative (error).

**recv() Error Handling:**
```cpp
// File: src/Client/Client.cpp, lines 102-136
ssize_t n = recv(_socket, buf, sizeof(buf), 0);

if (n > 0)
{
    _request_buffer.append(buf, buf + n);
    continue;
}

if (n == 0)
{
    // Connection closed by peer
    _peer_half_closed = true;
    break;
}

if (n < 0)
{
    if (errno == EINTR)
        continue;  // Interrupted, retry
        
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;  // No data available (non-blocking)
        
    // Fatal error
    _state = CLOSING;  // Mark for removal
    return;
}
```

**send() Error Handling:**
```cpp
// File: src/Client/Client.cpp, lines 380-411
ssize_t sent = send(_socket, ...);

if (sent > 0)
{
    _response_offset += sent;
    continue;
}

if (sent < 0)
{
    if (errno == EINTR)
        continue;  // Interrupted, retry
        
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;  // Buffer full, try later
        
    // Fatal error
    _state = CLOSING;
    return;
}
```

**Client Removal:**
```cpp
// File: src/server/AcceptSocket.cpp, lines 145-159
if (cl->getState() == CLOSING)
{
    toClose.push_back(cfd);
}

for (size_t i = 0; i < toClose.size(); ++i)
{
    close(toClose[i]);
    delete _clients[toClose[i]];
    _clients.erase(toClose[i]);
}
```

### Q7: Do you check errno independently after read/send?

**Answer:** **NO.** We ONLY check errno AFTER detecting an error (return value < 0).

**Correct (what we do):**
```cpp
ssize_t n = recv(_socket, buf, sizeof(buf), 0);
if (n < 0)  // First check: error occurred?
{
    // Now safe to check errno
    if (errno == EINTR) continue;
    if (errno == EAGAIN) break;
}
```

**Incorrect (what we DON'T do):**
```cpp
ssize_t n = recv(_socket, buf, sizeof(buf), 0);
if (errno == EAGAIN)  // WRONG! errno not reset on success
    return;
```

---

## Non-Blocking I/O

### Q8: How do you set sockets to non-blocking?

**Answer:** Using `fcntl()` with `O_NONBLOCK` flag.

**Code Reference:**
```cpp
// File: src/server/ServerUtils.cpp
bool HttpServer::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    
    flags |= O_NONBLOCK;
    
    if (fcntl(fd, F_SETFL, flags) == -1)
        return false;
    
    return true;
}
```

**Applied to:**
1. Server sockets after binding
2. Client sockets after accepting

### Q9: Why use non-blocking I/O?

**Answer:**
- Prevents one slow client from blocking entire server
- Works correctly with select()
- Enables concurrent handling
- Can read/write what's available and move on

---

## Client State Machine

### Q10: Explain your client state machine.

**Answer:** Each client has a state tracking its position in the request-response cycle.

**States:**
```cpp
enum ClientState
{
    READING,              // Reading HTTP request
    GENERATING_RESPONSE,  // Parsing and generating response
    WRITING,             // Sending HTTP response
    AWAITING_CGI,        // Waiting for CGI
    CLOSING              // Ready for cleanup
};
```

**State transitions:**
```cpp
// File: src/Client/HandleClient.cpp
void Client::handleConnection()
{
    switch (_state)
    {
        case READING:
            readRequest();  // -> GENERATING_RESPONSE when complete
            break;
        case GENERATING_RESPONSE:
            generateResponse();  // -> WRITING
            break;
        case WRITING:
            writeResponse();  // -> READING (keep-alive) or CLOSING
            break;
        case AWAITING_CGI:
            handleCgi();  // -> WRITING
            break;
        case CLOSING:
            break;  // Server will clean up
    }
}
```

**Flow:**
1. New client starts in READING
2. When request complete → GENERATING_RESPONSE
3. When response ready → WRITING
4. When sent: keep-alive → READING, else → CLOSING

### Q11: How do you handle keep-alive?

**Answer:** Keep-alive allows multiple requests on same connection.

**Determining keep-alive:**
```cpp
bool HttpServer::determineKeepAlive(const HTTPparser &parser)
{
    std::string connection = parser.getHeader("Connection");
    std::string version = parser.getVersion();
    
    // HTTP/1.1: keep-alive by default unless "Connection: close"
    if (version == "HTTP/1.1")
        return (connection != "close");
    
    // HTTP/1.0: close by default unless "Connection: keep-alive"
    return (connection == "keep-alive");
}
```

**Implementing:**
```cpp
// File: src/Client/Client.cpp, lines 408-428
if (_response_offset >= _response_buffer.size())
{
    if (_keep_alive)
    {
        // Reset for next request
        _request_buffer.clear();
        _response_buffer.clear();
        _response_offset = 0;
        _parser.reset();
        _state = READING;  // Back to reading
    }
    else
    {
        _state = CLOSING;  // Close connection
    }
}
```

---

## CGI Implementation

### Q12: How do you implement CGI?

**Answer:** CGI executes external scripts to generate dynamic content.

**CGI Detection:**
```cpp
// File: src/Client/Client.cpp, lines 262-285
const LocationConfig *loc = _server.getCurrentLocation();
bool cgiEnabled = (loc && loc->cgiPass);
bool methodAllowed = _server.isMethodAllowed(method);

if (cgiEnabled && methodAllowed)
{
    _parser.setCurrentFilePath(filePath);
    std::string cgiOut = _server.processCGI(_parser);
    
    if (cgiOut.compare(0, 5, "HTTP/") == 0)
        _response_buffer = cgiOut;  // Full HTTP response
    else
        _response_buffer = _server.generatePostResponse(cgiOut, _keep_alive);
    
    _state = WRITING;
}
```

**CGI Environment Variables:**
- REQUEST_METHOD (GET, POST, DELETE)
- PATH_INFO (script path)
- QUERY_STRING (URL parameters)
- CONTENT_LENGTH (POST data size)
- CONTENT_TYPE (POST data type)
- SERVER_NAME, SERVER_PORT

### Q13: How do you handle CGI errors?

**Answer:**
- **Script not found:** 404 Not Found
- **Not executable:** 500 Internal Server Error
- **Exit with error:** 500 Internal Server Error
- **Timeout:** Kill script, return 500
- **Invalid output:** 500 Internal Server Error

**Important:** CGI errors NEVER crash the server - always converted to HTTP error responses.

---

## Configuration

### Q14: How does your config parser work?

**Answer:** Parses nginx-like config files into data structures.

**Config structure:**
```nginx
server {
    listen 8080;
    server_name localhost;
    root /www/html;
    index index.html;
    client_max_body_size 1m;
    error_page 404 /404.html;
    
    location / {
        allowed_methods GET;
        autoindex off;
    }
    
    location /cgi-bin/ {
        allowed_methods GET POST DELETE;
        cgi_pass on;
        cgi_extension .py;
    }
}
```

**Parser workflow:**
1. Tokenization - Break into tokens
2. Validation - Check syntax
3. Building - Create config objects
4. Storage - Store for server use

### Q15: How do you handle multiple server blocks?

**Answer:** Support multiple servers with different ports/names.

**Code:**
```cpp
// File: src/server/HttpServer.cpp
for (size_t serverIdx = 0; serverIdx < _servers.size(); ++serverIdx)
{
    const ServerConfig &serverConfig = _servers[serverIdx];
    const std::vector<int> &ports = serverConfig.getListenPorts();
    
    for (size_t portIdx = 0; portIdx < ports.size(); ++portIdx)
    {
        int port = ports[portIdx];
        int server_fd = createAndBindSocket(port);
        
        // Store with server index for later matching
        _serverSockets.push_back(ServerSocketInfo(server_fd, port, serverIdx));
    }
}
```

**Virtual hosts (server name matching):**
```cpp
// File: src/server/HttpServer.cpp, lines 217-244
size_t HttpServer::selectServerForRequest(const HTTPparser &parser, int serverPort)
{
    std::string host = parser.getServerName();
    
    for (size_t i = 0; i < _servers.size(); ++i)
    {
        const ServerConfig &serverConfig = _servers[i];
        const std::string &serverName = serverConfig.getServerName();
        const std::vector<int> &listenPorts = serverConfig.getListenPorts();
        
        // Match server name AND port
        if (serverName == host)
        {
            for (size_t j = 0; j < listenPorts.size(); ++j)
            {
                if (listenPorts[j] == serverPort)
                    return i;  // Found matching server
            }
        }
    }
    
    return static_cast<size_t>(-1);  // No match
}
```

---

## Request Parsing

### Q16: How do you parse HTTP requests?

**Answer:** Multi-stage parsing: request line, headers, body.

**Request structure:**
```http
GET /path HTTP/1.1\r\n
Host: localhost\r\n
Connection: keep-alive\r\n
\r\n
[optional body]
```

**Parsing stages:**
```cpp
// File: src/httpParser/HTTPparser.cpp
bool HTTPparser::parseRequest(const std::string &request)
{
    // 1. Parse request line
    // GET /path HTTP/1.1
    size_t firstSpace = request.find(' ');
    size_t secondSpace = request.find(' ', firstSpace + 1);
    
    _method = request.substr(0, firstSpace);
    _path = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    _version = request.substr(secondSpace + 1, request.find("\r\n") - secondSpace - 1);
    
    // 2. Parse headers
    size_t pos = request.find("\r\n") + 2;
    while (true)
    {
        size_t lineEnd = request.find("\r\n", pos);
        if (lineEnd == pos) break;  // Empty line = end of headers
        
        std::string line = request.substr(pos, lineEnd - pos);
        size_t colon = line.find(':');
        
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 2);  // Skip ": "
        
        _headers[key] = value;
        pos = lineEnd + 2;
    }
    
    // 3. Parse body (if present)
    size_t bodyStart = request.find("\r\n\r\n") + 4;
    if (bodyStart < request.size())
    {
        _body = request.substr(bodyStart);
    }
    
    return validate();
}
```

### Q17: How do you validate requests?

**Answer:** Multiple validation checks.

**Validation:**
```cpp
bool HTTPparser::validate()
{
    // Check method
    if (_method != "GET" && _method != "POST" && _method != "DELETE")
    {
        _error = "Invalid method";
        return false;
    }
    
    // Check version
    if (_version != "HTTP/1.1" && _version != "HTTP/1.0")
    {
        _error = "Invalid HTTP version";
        return false;
    }
    
    // Check Host header (required in HTTP/1.1)
    if (_version == "HTTP/1.1" && _headers.find("Host") == _headers.end())
    {
        _error = "Missing Host header";
        return false;
    }
    
    // Check Content-Length for POST
    if (_method == "POST")
    {
        if (_headers.find("Content-Length") == _headers.end())
        {
            _error = "Missing Content-Length for POST";
            return false;
        }
    }
    
    return true;
}
```

---

## Response Generation

### Q18: How do you generate HTTP responses?

**Answer:** Build response string with status line, headers, and body.

**Response structure:**
```http
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 1234\r\n
Connection: keep-alive\r\n
\r\n
[body content]
```

**Code:**
```cpp
// File: src/httpResponse/Response.cpp
std::string HttpServer::generateGetResponse(const std::string &path, bool keepAlive)
{
    std::ostringstream response;
    
    // Check if file exists
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return generate404Response(keepAlive);
    }
    
    // Read file content
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Build response
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << getContentType(path) << "\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    response << "\r\n";
    response << content;
    
    return response.str();
}
```

### Q19: What HTTP status codes do you support?

**Answer:** Main status codes:

**Success:**
- 200 OK - Request succeeded
- 201 Created - Resource created (POST)
- 204 No Content - Success but no body (DELETE)

**Redirection:**
- 301 Moved Permanently
- 302 Found (temporary redirect)

**Client Errors:**
- 400 Bad Request - Malformed request
- 403 Forbidden - Access denied
- 404 Not Found - Resource doesn't exist
- 405 Method Not Allowed - Method not supported
- 413 Payload Too Large - Body exceeds limit

**Server Errors:**
- 500 Internal Server Error - Server fault
- 501 Not Implemented - Feature not supported
- 505 HTTP Version Not Supported

---

## Advanced Topics

### Q20: How do you prevent memory leaks?

**Answer:**
- Delete Client objects when closing
- Close file descriptors properly
- Clean up CGI pipes
- Clear buffers after use

**Code:**
```cpp
// Client cleanup
for (size_t i = 0; i < toClose.size(); ++i)
{
    int fd = toClose[i];
    close(fd);              // Close socket
    delete _clients[fd];    // Delete Client object
    _clients.erase(fd);     // Remove from map
}

// Client destructor
Client::~Client()
{
    // Close CGI pipes if any
    if (_cgi_pipe_in[0] != -1) close(_cgi_pipe_in[0]);
    if (_cgi_pipe_in[1] != -1) close(_cgi_pipe_in[1]);
    if (_cgi_pipe_out[0] != -1) close(_cgi_pipe_out[0]);
    if (_cgi_pipe_out[1] != -1) close(_cgi_pipe_out[1]);
}
```

### Q21: How do you handle slow clients?

**Answer:**
- Non-blocking I/O prevents blocking
- select() monitors multiple clients
- Slow client doesn't affect others
- Can implement timeouts if needed

### Q22: What happens if select() is interrupted by a signal?

**Answer:**
```cpp
int ready = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
if (ready < 0)
{
    if (errno == EINTR)
        continue;  // Interrupted by signal, retry
    // Other error
    break;
}
```

### Q23: How do you ensure no file descriptor leaks?

**Answer:**
- Every `open()` matched with `close()`
- Every `accept()` matched with `close()`
- Every `pipe()` matched with `close()` on both ends
- Client destructor closes all FDs
- Server shutdown closes all remaining FDs

---

## Quick Reference: Key Code Locations

| Feature | File | Line Range |
|---------|------|------------|
| Main select() loop | src/server/AcceptSocket.cpp | 18-77 |
| Client state machine | src/Client/HandleClient.cpp | 11-33 |
| recv() error handling | src/Client/Client.cpp | 102-136 |
| send() error handling | src/Client/Client.cpp | 380-411 |
| Non-blocking setup | src/server/ServerUtils.cpp | - |
| Server start | src/server/HttpServer.cpp | 125-173 |
| Config parsing | src/configParser/ConfigParser.cpp | - |
| Request parsing | src/httpParser/HTTPparser.cpp | - |
| Response generation | src/httpResponse/Response.cpp | - |
| CGI execution | src/CGI/CGI.cpp | - |

---

## Final Evaluation Tips

**Be ready to:**
1. Show the single select() call
2. Explain read_fds vs write_fds
3. Demonstrate error handling for recv/send
4. Show client state transitions
5. Explain keep-alive implementation
6. Demonstrate CGI execution
7. Show config parsing
8. Explain virtual host selection
9. Show memory cleanup
10. Run stress tests successfully

**Common pitfalls to avoid:**
- Checking errno without checking return value first
- Multiple select() calls
- Blocking I/O operations
- Not handling all recv/send return values
- Memory leaks in client cleanup
- CGI errors crashing server
- Not handling EINTR properly

---

## End of Q&A Document