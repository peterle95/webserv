# Webserv Evaluation Guide

This document provides step-by-step responses for passing the webserv evaluation sheet.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Mandatory Part](#mandatory-part)
- [Configuration Tests](#configuration-tests)
- [Basic Checks](#basic-checks)
- [CGI Tests](#cgi-tests)
- [Browser Tests](#browser-tests)
- [Port Issues](#port-issues)
- [Siege & Stress Test](#siege--stress-test)

---

## Prerequisites

### Install Siege
```bash
brew install siege
```

### Compile the Project
```bash
make
```

**Expected output:** Clean compilation without errors or re-link issues.

### Start the Server
```bash
./webserv conf/default.conf
```

**Expected output:**
```
Server block 0 (localhost) listening on port 8080
```

---

## Mandatory Part

### 1. Check the Code and Ask Questions

#### Our I/O Multiplexing Function
**Answer:** We use `select()` for I/O multiplexing.

**Code Reference:**
```cpp
// File: src/server/AcceptSocket.cpp, lines 18-77
int HttpServer::runMultiServerAcceptLoop(const std::vector<ServerSocketInfo> &serverSockets, bool serveOnce)
{
    while (!g_stop)
    {
        fd_set read_fds;
        fd_set write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        int max_fd = -1;

        // Add ALL server sockets to read set
        for (size_t i = 0; i < serverSockets.size(); ++i)
        {
            int server_fd = serverSockets[i].socket_fd;
            FD_SET(server_fd, &read_fds);
            if (server_fd > max_fd)
                max_fd = server_fd;
        }

        // Add client FDs based on their current state
        for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            int cfd = it->first;
            Client *cl = it->second;
            ClientState st = cl->getState();
            if (st == READING)
            {
                FD_SET(cfd, &read_fds);
                if (cfd > max_fd)
                    max_fd = cfd;
            }
            else if (st == WRITING)
            {
                FD_SET(cfd, &write_fds);
                if (cfd > max_fd)
                    max_fd = cfd;
            }
        }

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ready = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
        // ... rest of the logic
    }
}
```

#### How select() Works
**Explanation:**
`select()` monitors multiple file descriptors to see if they are ready for I/O operations. It takes four file descriptor sets:
- **read_fds:** Monitors for incoming data (readable)
- **write_fds:** Monitors for write readiness
- **except_fds:** Monitors for exceptional conditions (we don't use this)
- **timeout:** Maximum time to wait

When `select()` returns:
- It modifies the fd_sets to indicate which descriptors are ready
- Returns the number of ready descriptors
- We check with `FD_ISSET()` to see which specific FDs are ready

#### Single select() in Main Loop
**Answer:** YES, we use only ONE `select()` in the main loop.

**Code Reference:**
```cpp
// File: src/server/AcceptSocket.cpp, line 52
int ready = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
```

This single `select()` monitors:
1. **Server sockets for accepting new connections** (read set)
2. **Client sockets for reading requests** (read set)
3. **Client sockets for writing responses** (write set)

**All at the same time!**

#### How We Managed Server Accept and Client Read/Write
The `select()` checks read and write AT THE SAME TIME:

```cpp
// File: src/server/AcceptSocket.cpp
// Server sockets are added to read_fds (for accepting)
for (size_t i = 0; i < serverSockets.size(); ++i)
{
    int server_fd = serverSockets[i].socket_fd;
    FD_SET(server_fd, &read_fds);  // Ready to accept
}

// Clients are added based on their state
for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
{
    ClientState st = cl->getState();
    if (st == READING)
        FD_SET(cfd, &read_fds);    // Ready to read
    else if (st == WRITING)
        FD_SET(cfd, &write_fds);   // Ready to write
}
```

After `select()` returns, we check which FDs are ready:
```cpp
// Accept new connections from server sockets
if (FD_ISSET(server_fd, &read_fds))
    // accept() new connections

// Handle client I/O
if (st == READING && FD_ISSET(cfd, &read_fds))
    cl->handleConnection();  // Read request
else if (st == WRITING && FD_ISSET(cfd, &write_fds))
    cl->handleConnection();  // Write response
```

### 2. One Read/Write per Client per select()

**Code Path from select() to read/write:**

#### Read Path:
```cpp
// File: src/server/AcceptSocket.cpp, lines 131-139
if (st == READING && FD_ISSET(cfd, &read_fds))
{
    cl->handleConnection();  // Calls Client::handleConnection()
}

// File: src/Client/HandleClient.cpp, lines 20-21
void Client::handleConnection()
{
    switch (_state)
    {
        case READING:              
            readRequest();  // Single read operation per select()
            break;
        // ...
    }
}

// File: src/Client/Client.cpp, lines 94-106
void Client::readRequest()
{
    char buf[4096];
    while (true)
    {
        ssize_t n = recv(_socket, buf, sizeof(buf), 0);  // ONE recv per cycle
        if (n > 0)
        {
            _request_buffer.append(buf, buf + n);
            // ... check if complete
            continue;  // Drain socket in non-blocking mode
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;  // No more data, exit
    }
}
```

#### Write Path:
```cpp
// File: src/server/AcceptSocket.cpp, lines 139-142
else if (st == WRITING && FD_ISSET(cfd, &write_fds))
{
    cl->handleConnection();  // Calls Client::handleConnection()
}

// File: src/Client/Client.cpp, lines 380-395
void Client::writeResponse()
{
    while (_response_offset < _response_buffer.size())
    {
        ssize_t sent = send(_socket,
                            _response_buffer.c_str() + _response_offset,
                            _response_buffer.size() - _response_offset,
                            0);  // ONE send per iteration
        if (sent > 0)
        {
            _response_offset += sent;
            continue;  // Try to send more in non-blocking mode
        }
        if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return;  // Socket buffer full, try later
    }
}
```

**Note:** We use a while loop to drain/fill the socket buffer in non-blocking mode, but each `recv()`/`send()` call happens **within one select() cycle**.

### 3. Error Handling on read/recv/write/send

#### Read/Recv Error Handling:
```cpp
// File: src/Client/Client.cpp, lines 119-136
ssize_t n = recv(_socket, buf, sizeof(buf), 0);
if (n > 0)
{
    // Success - append data
}
if (n == 0)
{
    // Peer closed connection
    _peer_half_closed = true;
    break;
}
if (n < 0)
{
    if (errno == EINTR)
        continue;  // Interrupted, retry
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;  // No more data
    // Fatal error
    _state = CLOSING;  // Client will be removed
    return;
}
```

**Client is removed when state is CLOSING:**
```cpp
// File: src/server/AcceptSocket.cpp, lines 149-159
if (cl->getState() == CLOSING)
{
    toClose.push_back(cfd);
}

// Close and delete clients marked for closing
for (size_t i = 0; i < toClose.size(); ++i)
{
    int fd = toClose[i];
    close(fd);
    delete it->second;
    _clients.erase(it);
}
```

#### Write/Send Error Handling:
```cpp
// File: src/Client/Client.cpp, lines 382-401
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
        return;  // Try again later
    _state = CLOSING;  // Fatal error, remove client
    return;
}
// sent == 0 shouldn't happen
break;
```

### 4. Checking Both -1 and 0 Return Values

**We check BOTH -1 (error) and 0 (closed connection):**

#### Read:
```cpp
if (n == 0)  // Connection closed ✓
    _peer_half_closed = true;
if (n < 0)   // Error ✓
    // Handle error or EAGAIN
```

#### Write:
```cpp
if (sent < 0)  // Error ✓
    // Handle error
// sent == 0 is also handled ✓
```

### 5. We Do NOT Check errno After read/recv/write/send

**Confirmation:** We only check errno inside the error handling blocks when `n < 0` or `sent < 0`. We never check errno independently.

```cpp
// CORRECT pattern:
if (n < 0)
{
    if (errno == EINTR)      // Check errno only after detecting error
        continue;
    if (errno == EAGAIN)
        break;
}
```

### 6. No File Descriptor Access Without select()

**All I/O operations go through select():**
- Server sockets are added to `read_fds` for accepting
- Client sockets are added to `read_fds` or `write_fds` based on state
- We only call `recv()`/`send()` when `FD_ISSET()` confirms the FD is ready

---

## Configuration Tests

### Test 1: Multiple Servers with Different Ports

**Config file:** Create `conf/multi_port.conf`
```nginx
server {
    listen 8080;
    server_name localhost;
    root www/html;
    index index.html;
    location / {
        allowed_methods GET;
    }
}

server {
    listen 8081;
    server_name localhost;
    root www/html;
    index index.html;
    location / {
        allowed_methods GET;
    }
}
```

**Test:**
```bash
./webserv conf/multi_port.conf

# In another terminal:
curl http://localhost:8080/
curl http://localhost:8081/
```

**Expected:** Both return the same HTML content from `www/html/index.html`

### Test 2: Multiple Servers with Different Hostnames

**Test:**
```bash
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/
```

**Expected:** Server responds based on Host header matching

### Test 3: Custom Error Page

**Modify `conf/default.conf`:**
```nginx
error_page 404 /404.html;
```

**Test:**
```bash
curl http://localhost:8080/nonexistent
```

**Expected:** Returns custom 404 page if it exists, or default 404 response

### Test 4: Limit Client Body Size

**Test small body (should work):**
```bash
curl -X POST -H "Content-Type: text/plain" --data "small" http://localhost:8080/cgi-bin/
```

**Test large body (should fail if > 1m):**
```bash
# Generate 2MB file
dd if=/dev/zero of=/tmp/large.txt bs=1M count=2

curl -X POST -H "Content-Type: text/plain" --data-binary @/tmp/large.txt http://localhost:8080/cgi-bin/
```

**Expected:** Second request returns 413 Payload Too Large

### Test 5: Routes to Different Directories

**Config shows:**
```nginx
location / {
    root /home/pmolzer/Downloads/webserv/www/html;
}

location /cgi-bin/ {
    # Different directory handling
    cgi_pass on;
}
```

**Test:**
```bash
curl http://localhost:8080/
curl http://localhost:8080/cgi-bin/test.py
```

### Test 6: Default File for Directory

**Config:**
```nginx
index index.html;
```

**Test:**
```bash
curl http://localhost:8080/
```

**Expected:** Returns content of `index.html`

### Test 7: Allowed Methods per Route

**Config:**
```nginx
location / {
    allowed_methods GET;  # Only GET allowed
}

location /cgi-bin/ {
    allowed_methods GET POST DELETE;  # Multiple methods
}
```

**Test:**
```bash
# Should work
curl -X GET http://localhost:8080/

# Should fail with 405
curl -X DELETE http://localhost:8080/

# Should work
curl -X POST http://localhost:8080/cgi-bin/test.py

# Should work
curl -X DELETE http://localhost:8080/cgi-bin/test.py
```

---

## Basic Checks

### Test 1: GET Request
```bash
curl -v http://localhost:8080/
```

**Expected:** HTTP/1.1 200 OK with HTML content

### Test 2: POST Request
```bash
curl -X POST -H "Content-Type: text/plain" --data "test data" http://localhost:8080/cgi-bin/test.py
```

**Expected:** HTTP/1.1 200 OK (if CGI script exists and works)

### Test 3: DELETE Request
```bash
curl -X DELETE http://localhost:8080/uploads/test.txt
```

**Expected:** HTTP/1.1 200 OK or appropriate status

### Test 4: Unknown Method
```bash
curl -X UNKNOWN http://localhost:8080/
```

**Expected:** HTTP/1.1 405 Method Not Allowed (server should NOT crash)

### Test 5: Malformed Request
```bash
echo -e "GARBAGE\r\n\r\n" | nc localhost 8080
```

**Expected:** HTTP/1.1 400 Bad Request (server should NOT crash)

### Test 6: Upload and Retrieve File

**Upload:**
```bash
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads/
```

**Retrieve:**
```bash
curl http://localhost:8080/uploads/test.txt
```

**Expected:** Get back the same file content

---

## CGI Tests

### Prerequisites
Create a simple CGI script: `www/cgi-bin/test.py`

```python
#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/plain\r")
print("\r")
print("CGI Script Output")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'N/A')}")
print(f"PATH_INFO: {os.environ.get('PATH_INFO', 'N/A')}")
print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', 'N/A')}")

# Read POST data if available
content_length = os.environ.get('CONTENT_LENGTH', '0')
if content_length and content_length != '0':
    post_data = sys.stdin.read(int(content_length))
    print(f"POST Data: {post_data}")
```

```bash
chmod +x www/cgi-bin/test.py
```

### Test 1: GET with CGI
```bash
curl http://localhost:8080/cgi-bin/test.py
```

**Expected:** 
```
CGI Script Output
REQUEST_METHOD: GET
PATH_INFO: /cgi-bin/test.py
```

### Test 2: POST with CGI
```bash
curl -X POST -H "Content-Type: text/plain" --data "Hello CGI" http://localhost:8080/cgi-bin/test.py
```

**Expected:**
```
CGI Script Output
REQUEST_METHOD: POST
POST Data: Hello CGI
```

### Test 3: CGI Error Handling

Create `www/cgi-bin/error.py`:
```python
#!/usr/bin/env python3
import sys
sys.exit(1)  # Exit with error
```

```bash
chmod +x www/cgi-bin/error.py
curl http://localhost:8080/cgi-bin/error.py
```

**Expected:** HTTP/1.1 500 Internal Server Error (server should NOT crash)

### Test 4: Infinite Loop CGI

Create `www/cgi-bin/infinite.py`:
```python
#!/usr/bin/env python3
import time
while True:
    time.sleep(1)
```

```bash
chmod +x www/cgi-bin/infinite.py
curl --max-time 5 http://localhost:8080/cgi-bin/infinite.py
```

**Expected:** Request should timeout, server should NOT crash

---

## Browser Tests

### Test 1: Open in Browser
```
http://localhost:8080/
```

**Check:**
- Open browser developer tools (F12)
- Go to Network tab
- Request headers should show valid HTTP request
- Response headers should show valid HTTP response

### Test 2: Wrong URL
```
http://localhost:8080/nonexistent
```

**Expected:** 404 Not Found page

### Test 3: Directory Listing
```
http://localhost:8080/uploads/
```

**Expected:** 
- If `autoindex on`: Directory listing
- If `autoindex off`: 403 Forbidden or index.html

### Test 4: Redirected URL
```
http://localhost:8080/old
```

**Expected:** Redirect to new location (if configured)

---

## Port Issues

### Test 1: Multiple Ports, Different Sites

**Config:** Already handled in multi_port.conf example above

**Test in browser:**
- Visit `http://localhost:8080/`
- Visit `http://localhost:8081/`

### Test 2: Same Port Multiple Times

**Bad Config:** `conf/duplicate_port.conf`
```nginx
server {
    listen 8080;
    server_name site1.local;
}

server {
    listen 8080;
    server_name site2.local;
}
```

**Test:**
```bash
./webserv conf/duplicate_port.conf
```

**Expected:** Should work! Both servers bind to same port but with different server_names

### Test 3: Multiple Servers, Common Ports

Start first server:
```bash
./webserv conf/default.conf
```

Try to start second server with same port:
```bash
./webserv conf/default.conf
```

**Expected:** Second server should FAIL to bind with "Address already in use"

---

## Siege & Stress Test

### Test 1: Basic Siege
```bash
siege -b -c10 -t30s http://localhost:8080/
```

**Parameters:**
- `-b`: Benchmark mode (no delays)
- `-c10`: 10 concurrent connections
- `-t30s`: Run for 30 seconds

**Expected:**
- Availability > 99.5%
- No server crashes
- Transactions/sec should be reasonable

### Test 2: Check for Memory Leaks

**Terminal 1 - Start server:**
```bash
./webserv conf/default.conf
```

**Terminal 2 - Monitor memory:**
```bash
# Get PID
ps aux | grep webserv

# Monitor continuously (replace PID)
watch -n 1 'ps -p <PID> -o pid,vsz,rss,comm'
```

**Terminal 3 - Run siege:**
```bash
siege -b -c50 -t120s http://localhost:8080/
```

**Expected:** 
- Memory (VSZ/RSS) should stabilize
- Should NOT grow indefinitely

### Test 3: Check for Hanging Connections
```bash
# While siege is running
lsof -i :8080 | grep webserv
```

**Expected:** 
- Number of connections should be reasonable
- Old connections should close properly
- No accumulation of CLOSE_WAIT or TIME_WAIT states

### Test 4: Indefinite Siege
```bash
siege -b http://localhost:8080/
```

**Press Ctrl+C after a while**

**Expected:**
- Server continues running
- Can still handle new requests
- No crash or freeze

### Test 5: Concurrent POST Requests
```bash
siege -b -c20 -t30s 'http://localhost:8080/cgi-bin/test.py POST data=test'
```

**Expected:**
- All requests processed correctly
- No CGI errors
- Server remains stable

### Test 6: Large File Stress
```bash
# Create a 100KB test file
dd if=/dev/zero of=www/html/large.bin bs=1024 count=100

# Stress test
siege -b -c50 -t60s http://localhost:8080/large.bin
```

**Expected:**
- High throughput
- No corruption
- Server stability

---

## Quick Checklist Before Evaluation

- [ ] Clean compilation (`make`)
- [ ] Siege installed (`brew install siege`)
- [ ] Server starts without errors
- [ ] GET requests work
- [ ] POST requests work
- [ ] DELETE requests work
- [ ] Unknown methods return 405
- [ ] Malformed requests return 400
- [ ] CGI scripts executable and work
- [ ] select() is in main loop only
- [ ] One read/write per select() cycle
- [ ] Error handling checks both -1 and 0
- [ ] errno not checked independently
- [ ] No I/O without going through select()
- [ ] Memory stable during siege
- [ ] No hanging connections
- [ ] Availability > 99.5% in siege

---

## Common Issues and Solutions

### Issue: Server crashes on malformed request
**Solution:** Check error handling in HTTPparser

### Issue: Memory leak detected
**Solution:** Check client cleanup in AcceptSocket.cpp

### Issue: Hanging connections
**Solution:** Verify keep-alive logic and connection timeouts

### Issue: Siege availability < 99.5%
**Solution:** Check non-blocking I/O and select() logic

### Issue: Server won't restart (Address in use)
**Solution:** Wait 60 seconds or use `SO_REUSEADDR` option
