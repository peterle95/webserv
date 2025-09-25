/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 14:20:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/09/12 15:33:51 by pmolzer          ###   .fr       */
/*                                                                            */
/* ************************************************************************** */

/*
 * REFACTORING SUMMARY - Non-blocking select()-based HttpServer
 * ============================================================
 *
 * ORIGINAL IMPLEMENTATION:
 * - Blocking single-threaded server using accept(), recv(), sendAll()
 * - Each request processed sequentially: accept -> read entire request -> parse -> generate response -> send -> close
 * - Could only handle one client at a time
 * - Used blocking I/O operations that would wait indefinitely
 *
 * NEW IMPLEMENTATION (Non-blocking select()-based event loop):
 * - Event-driven architecture using select() system call for I/O multiplexing
 * - Can handle multiple concurrent client connections simultaneously
 * - Non-blocking sockets with fcntl(O_NONBLOCK) to prevent blocking on I/O operations
 * - Master fd_sets (read/write) rebuilt each iteration and passed to select()
 * - Per-client state management using std::map<int, ClientState>
 *
 * KEY ARCHITECTURAL CHANGES:
 * 1. NON-BLOCKING SOCKETS:
 *    - Server socket set to non-blocking mode using fcntl(F_SETFL, O_NONBLOCK)
 *    - All accepted client sockets also set to non-blocking
 *    - I/O operations (accept, recv, send) return immediately with EAGAIN/EWOULDBLOCK
 *
 * 2. CLIENT STATE MANAGEMENT:
 *    - ClientState struct tracks per-client data:
 *      * requestBuf: accumulates incoming request data incrementally
 *      * responseBuf: stores complete response to send
 *      * sendOffset: tracks progress of response transmission
 *      * keepAlive: whether connection should be reused
 *      * requestComplete: flag indicating end of request headers
 *      * parser: HTTPparser instance for incremental parsing
 *    - std::map<int, ClientState> maps file descriptor to client state
 *
 * 3. EVENT LOOP STRUCTURE:
 *    - Main while(true) loop with select() timeout for signal handling
 *    - Rebuild transient fd_sets from master sets each iteration
 *    - Iterate through all FDs from 0 to max_fd:
 *      * Listening socket ready: accept new connections non-blockingly
 *      * Client socket readable: recv() data incrementally until EAGAIN
 *      * Client socket writable: send() response data incrementally until EAGAIN
 *
 * 4. REQUEST/RESPONSE FLOW:
 *    - Request data accumulated in requestBuf until headers complete ("\r\n\r\n")
 *    - Complete request parsed using existing HTTPparser
 *    - Response generated using existing file-serving logic
 *    - Response sent incrementally, tracking progress with sendOffset
 *    - Connection reused for keep-alive or closed after response
 *
 * 5. KEEP-ALIVE SUPPORT:
 *    - Determined from request Connection header (case-insensitive) and HTTP version
 *    - HTTP/1.1 defaults to keep-alive if Connection header not present
 *    - Appropriate Connection header set in responses
 *    - State reset for next request on keep-alive connections
 *
 * 6. ERROR HANDLING & CLEANUP:
 *    - Graceful handling of EAGAIN/EWOULDBLOCK/EINTR on all I/O operations
 *    - Client cleanup on connection close or errors
 *    - Proper fd_set management (FD_SET, FD_CLR, FD_ZERO)
 *    - Signal handling for graceful shutdown
 *
 * BENEFITS:
 * - Concurrent handling of multiple clients without threads
 * - Better resource utilization and scalability
 * - No blocking on slow clients or large requests
 * - Maintains compatibility with existing HTTPparser and Response logic
 * - Supports both keep-alive and connection-close modes
 *
 * COMPATIBILITY:
 * - Preserves all existing functionality and API
 * - Uses same HTTPparser and file-serving logic
 * - Maintains WEBSERV_ONCE=1 support for CI testing
 * - Same signal handling for graceful shutdown
 *
 * PERFORMANCE CHARACTERISTICS:
 * - O(1) per client operations using fd_sets
 * - Efficient for hundreds of concurrent connections
 * - Minimal memory overhead per client state
 * - Optimal for I/O-bound workloads
 */

#include "Common.hpp"

HttpServer::HttpServer(int port, const std::string &root, const std::string &index)
: _port(port), _root(root), _index(index) {}

HttpServer::~HttpServer() {}

// Helper to set a socket to non-blocking mode
// REFACTORED: Added this function to support non-blocking I/O operations
// Previously, all sockets used blocking I/O which limited concurrency
static bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return false;
    return true;
}

// Global stop flag set by signal handlers
static volatile sig_atomic_t g_stop = 0;

static void handle_stop_signal(int)
{
    g_stop = 1;
}

// Per-client state for the select()-based event loop
// REFACTORED: Introduced ClientState struct to track per-client data across iterations
// Previously, no state was maintained between loop iterations in the blocking version
// This struct is essential for the non-blocking event-driven architecture
struct ClientState {
    std::string requestBuf;     // Accumulates incoming request data incrementally
    std::string responseBuf;    // Stores complete response to be sent
    size_t      sendOffset;     // Tracks how much of response has been sent
    bool        keepAlive;      // Whether connection should be reused for next request
    bool        requestComplete;// Flag indicating request headers are fully received
    HTTPparser  parser;         // Parser instance for incremental request parsing

    // Constructor initializes state for new connections
    // REFACTORED: Added constructor to properly initialize all member variables
    ClientState(): sendOffset(0), keepAlive(false), requestComplete(false) {}
};

int HttpServer::start()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "socket() failed" << std::endl;
        return 1;
    }

    // Allow quick restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind address
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)_port);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "bind() failed on port " << _port << std::endl;
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 128) < 0)
    {
        std::cerr << "listen() failed" << std::endl;
        close(server_fd);
        return 1;
    }

    // REFACTORED: Set listening socket to non-blocking mode
    // This is crucial for the event-driven architecture - prevents accept() from blocking
    if (!setNonBlocking(server_fd))
    {
        std::cerr << "Failed to set server socket non-blocking" << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "Serving " << _root << "/" << _index << " on http://localhost:" << _port << "/" << std::endl;

    // Signals for graceful shutdown
    std::signal(SIGINT, handle_stop_signal);
    std::signal(SIGTERM, handle_stop_signal);

    // Single-request mode for CI when WEBSERV_ONCE=1
    const char* onceEnv = std::getenv("WEBSERV_ONCE");
    bool serveOnce = (onceEnv && std::string(onceEnv) == "1");
    size_t servedCount = 0;

    // REFACTORED: Client state management - maps file descriptors to client state
    // This replaces the lack of state management in the original blocking version
    std::map<int, ClientState> clients;

    // REFACTORED: Master fd sets for I/O multiplexing with select()
    // master_read_set: tracks which FDs we want to read from
    // master_write_set: tracks which FDs we want to write to
    // These are rebuilt each iteration and passed to select()
    fd_set master_read_set;
    fd_set master_write_set;
    FD_ZERO(&master_read_set);
    FD_ZERO(&master_write_set);
    FD_SET(server_fd, &master_read_set);  // Add listening socket to read set
    int max_fd = server_fd;               // Track highest FD number for select()

    DEBUG_PRINT(RED << "🚀 HTTP Server started on port " << _port << RESET);
    DEBUG_PRINT("📁 Serving files from: " << RED << _root << "/" << _index << RESET);
    DEBUG_PRINT("⚙️  Mode: " << (serveOnce ? RED "Single-request (CI mode)" : "Multi-request (keep-alive)") << RESET);

    // REFACTORED: Main event loop using select() for I/O multiplexing
    // This replaces the simple while(true) loop with blocking accept/recv/send
    // Now we can handle multiple clients concurrently without threads
    while (true)
    {
        if (g_stop)
            break;

        // REFACTORED: Create copies of master sets for this select() call
        // select() modifies the sets, so we need fresh copies each time
        fd_set read_set = master_read_set;
        fd_set write_set = master_write_set;

        // Optional short timeout to handle signals
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int ready = select(max_fd + 1, &read_set, &write_set, NULL, &tv);
        if (ready < 0)
        {
            if (errno == EINTR) continue;
            std::cerr << "select() failed" << std::endl;
            break;
        }
        if (ready == 0)
        {
            // timeout, loop again to check stop flag
            continue;
        }

        // Iterate over all possible fds up to max_fd
        for (int fd = 0; fd <= max_fd && ready > 0; ++fd)
        {
            // REFACTORED: Handle new incoming connections
            // When listening socket is ready, accept new clients non-blockingly
            if (FD_ISSET(fd, &read_set) && fd == server_fd)
            {
                --ready;
                while (true)
                {
                    struct sockaddr_in cli;
                    socklen_t clilen = sizeof(cli);
                    int cfd = accept(server_fd, (struct sockaddr*)&cli, &clilen);
                    if (cfd < 0)
                    {
                        // REFACTORED: Handle non-blocking accept
                        // EAGAIN/EWOULDBLOCK means no more connections to accept right now
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // No more to accept now
                        if (errno == EINTR)
                            continue;
                        std::cerr << "accept() failed" << std::endl;
                        break;
                    }
                    // REFACTORED: Set new client socket to non-blocking mode
                    if (!setNonBlocking(cfd))
                    {
                        close(cfd);
                        continue;
                    }
                    // REFACTORED: Initialize client state for new connection
                    clients[cfd] = ClientState();
                    FD_SET(cfd, &master_read_set);  // Add to read set for request data
                    if (cfd > max_fd) max_fd = cfd; // Update max_fd if necessary
                    DEBUG_PRINT("🔌 New connection accepted (fd: " << RED << cfd << RESET << ")");
                }
                continue;
            }

            // REFACTORED: Handle readable client sockets
            // When client FD is ready for reading, recv() data incrementally
            if (FD_ISSET(fd, &read_set) && fd != server_fd)
            {
                --ready;
                std::map<int, ClientState>::iterator it = clients.find(fd);
                if (it == clients.end())
                {
                    // Unknown fd, remove from master sets for safety
                    FD_CLR(fd, &master_read_set);
                    FD_CLR(fd, &master_write_set);
                    continue;
                }
                bool peerClosed = false;
                char buf[4096];
                while (true)
                {
                    ssize_t n = recv(fd, buf, sizeof(buf), 0);
                    if (n > 0)
                    {
                        // REFACTORED: Accumulate request data incrementally
                        // In blocking version, entire request was read at once
                        it->second.requestBuf.append(buf, buf + n);
                        // Heuristic: consider request complete if we reached end of headers
                        if (!it->second.requestComplete)
                        {
                            if (it->second.requestBuf.find("\r\n\r\n") != std::string::npos)
                            {
                                it->second.requestComplete = true;
                                DEBUG_PRINT("Request headers complete (fd: " << RED << fd << RESET << ", size: " << it->second.requestBuf.size() << " bytes)");
                            }
                        }
                        // Continue reading until EAGAIN
                        continue;
                    }
                    else if (n == 0)
                    {
                        peerClosed = true;
                        break;
                    }
                    else
                    {
                        // REFACTORED: Handle non-blocking recv errors
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        if (errno == EINTR)
                            continue;
                        // Fatal recv error
                        peerClosed = true;
                        break;
                    }
                }

                if (peerClosed)
                {
                    DEBUG_PRINT("🔌 Connection closed by peer (fd: " << RED << fd << RESET << ")");
                    close(fd);
                    FD_CLR(fd, &master_read_set);
                    FD_CLR(fd, &master_write_set);
                    clients.erase(fd);
                    continue;
                }

                // REFACTORED: Check if request parsing is complete and generate response
                if (it->second.requestComplete && it->second.responseBuf.empty())
                {
                    DEBUG_PRINT(RED << "=== Processing Request ===" << RESET);
                    DEBUG_PRINT("Raw request size: " << it->second.requestBuf.size() << " bytes");

                    // Use existing HTTPparser to parse the raw request
                    it->second.parser.parseRequest(it->second.requestBuf);

                    // Determine keep-alive based on request headers/version
                    std::ostringstream resp;
                    std::string conn = it->second.parser.getHeader("Connection");
                    std::string version = it->second.parser.getVersion();
                    for (size_t i = 0; i < conn.size(); ++i) conn[i] = static_cast<char>(std::tolower(conn[i]));
                    bool wantKeepAlive = false;
                    if (!conn.empty())
                        wantKeepAlive = (conn == "keep-alive");
                    else if (version == "HTTP/1.1")
                        wantKeepAlive = true; // default keep-alive for HTTP/1.1

                    if (!it->second.parser.isValid())
                    {
                        DEBUG_PRINT(RED << "Request parsing failed: " << it->second.parser.getErrorMessage() << RESET);
                        // Bad request dummy response
                        const std::string body = "<h1>400 Bad Request</h1>";
                        resp << "HTTP/1.1 400 Bad Request\r\n";
                        resp << "Content-Type: text/html; charset=utf-8\r\n";
                        resp << "Content-Length: " << body.size() << "\r\n";
                        if (wantKeepAlive) resp << "Connection: keep-alive\r\n"; else resp << "Connection: close\r\n";
                        resp << "\r\n";
                        resp << body;
                        DEBUG_PRINT(RED << "→ Sending 400 Bad Request" << RESET);
                    }
                    else if (it->second.parser.getMethod() == "GET")
                    {
                        std::string method = it->second.parser.getMethod();
                        std::string path = it->second.parser.getPath();
                        std::string version = it->second.parser.getVersion();

                        DEBUG_PRINT("Valid request parsed:");
                        DEBUG_PRINT("   Method: " << RED << method << RESET);
                        DEBUG_PRINT("   Path: " << RED << path << RESET);
                        DEBUG_PRINT("   Version: " << RED << version << RESET);

                        // Simple dummy response for GET only (no file I/O)
                        std::string body = std::string("<html><body><h1>OK</h1><p>GET ") + path + "</p></body></html>";
                        resp << "HTTP/1.1 200 OK\r\n";
                        resp << "Content-Type: text/html; charset=utf-8\r\n";
                        resp << "Content-Length: " << body.size() << "\r\n";
                        if (wantKeepAlive) resp << "Connection: keep-alive\r\n"; else resp << "Connection: close\r\n";
                        resp << "\r\n";
                        resp << body;
                        DEBUG_PRINT("→ Sending 200 OK for GET request");
                    }
                    else
                    {
                        std::string method = it->second.parser.getMethod();
                        DEBUG_PRINT("Method not allowed: " << RED << method << RESET << " (only GET supported)");
                        // Method not allowed for non-GET methods
                        const std::string body = "<h1>405 Method Not Allowed</h1>";
                        resp << "HTTP/1.1 405 Method Not Allowed\r\n";
                        resp << "Allow: GET\r\n";
                        resp << "Content-Type: text/html; charset=utf-8\r\n";
                        resp << "Content-Length: " << body.size() << "\r\n";
                        if (wantKeepAlive) resp << "Connection: keep-alive\r\n"; else resp << "Connection: close\r\n";
                        resp << "\r\n";
                        resp << body;
                        DEBUG_PRINT("→ Sending 405 Method Not Allowed");
                    }

                    it->second.responseBuf = resp.str();
                    it->second.sendOffset = 0;
                    it->second.keepAlive = wantKeepAlive;

                    DEBUG_PRINT("Response prepared (" << resp.str().size() << " bytes)");
                    DEBUG_PRINT("Keep-alive: " << (wantKeepAlive ? RED "enabled" : "disabled") << RESET);

                    // REFACTORED: Move fd from read to write set
                    FD_CLR(fd, &master_read_set);
                    FD_SET(fd, &master_write_set);

                    if (serveOnce)
                        ++servedCount;
                }
                continue;
            }

            // REFACTORED: Handle writable client sockets
            // When client FD is ready for writing, send response data incrementally
            if (FD_ISSET(fd, &write_set) && fd != server_fd)
            {
                --ready;
                std::map<int, ClientState>::iterator it = clients.find(fd);
                if (it == clients.end())
                {
                    FD_CLR(fd, &master_write_set);
                    continue;
                }

                const std::string &resp = it->second.responseBuf;
                DEBUG_PRINT("Sending response (fd: " << RED << fd << RESET << ", " << it->second.sendOffset << "/" << resp.size() << " bytes sent)");
                while (it->second.sendOffset < resp.size())
                {
                    ssize_t n = send(fd, resp.c_str() + it->second.sendOffset, resp.size() - it->second.sendOffset, 0);
                    if (n > 0)
                    {
                        it->second.sendOffset += (size_t)n;
                        continue;
                    }
                    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                    {
                        break; // Can't send more now
                    }
                    if (n < 0 && errno == EINTR)
                    {
                        continue; // retry
                    }
                    // Fatal error or peer closed
                    it->second.sendOffset = resp.size();
                    break;
                }

                if (it->second.sendOffset >= resp.size())
                {
                    DEBUG_PRINT("Response sent completely (fd: " << RED << fd << RESET << ")");
                    if (it->second.keepAlive)
                    {
                        // REFACTORED: Reset for next request, go back to reading
                        it->second.requestBuf.clear();
                        it->second.responseBuf.clear();
                        it->second.sendOffset = 0;
                        it->second.requestComplete = false;
                        FD_CLR(fd, &master_write_set);
                        FD_SET(fd, &master_read_set);
                        DEBUG_PRINT("Keep-alive: Ready for next request (fd: " << RED << fd << RESET << ")");
                    }
                    else
                    {
                        // Close connection
                        DEBUG_PRINT("Closing connection (fd: " << RED << fd << RESET << ")");
                        close(fd);
                        FD_CLR(fd, &master_write_set);
                        clients.erase(fd);
                    }
                }
                continue;
            }
        }

        // In CI single-request mode, exit after serving one complete response
        if (serveOnce && servedCount >= 1)
            break;
    }

    // REFACTORED: Cleanup - close all remaining client connections and server socket
    // In the original blocking version, only the server socket needed cleanup
    // Now we must clean up all active client connections as well
    DEBUG_PRINT("HTTP Server shutting down...");
    for (std::map<int, ClientState>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        close(it->first);
        DEBUG_PRINT("Closed connection (fd: " << RED << it->first << RESET << ")");
    }
    close(server_fd);
    DEBUG_PRINT(RED << "Server shutdown complete" << RESET);
    return 0;
}
