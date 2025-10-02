#include "Common.hpp"
#include "HttpServer.hpp"

/*
Waits for incoming connections on the server socket using select().
Uses a timeout of 1 second to allow for graceful shutdown.
*/
static int waitForConnection(int server_fd)
{
    // I/O multiplexing setup using fd_set for efficient connection monitoring
    // Instead of blocking on accept(), we use select() to wait for readable sockets
    // This allows the server to respond to shutdown signals and implement timeouts
    fd_set read_fds;                    // File descriptor set for monitoring readable sockets
    FD_ZERO(&read_fds);                 // Initialize/clear the file descriptor set to avoid garbage data
    FD_SET(server_fd, &read_fds);       // Add server socket to the set for read monitoring

    struct timeval tv;
    tv.tv_sec = 1;                      // Set 1-second timeout to periodically check for shutdown
    tv.tv_usec = 0;                     // No microseconds, exactly 1 second

    // select() will block until:
    // 1) Server socket has incoming connection (readable)
    // 2) Timeout expires (1 second)
    // 3) Interrupted by signal (EINTR)
    // Returns number of ready file descriptors, 0 on timeout, -1 on error
    int ready = select(server_fd + 1, &read_fds, NULL, NULL, &tv);
    if (ready < 0 && errno != EINTR)
        std::cerr << "select() on server socket failed" << std::endl;

    return ready;
}

/*
Accepts an incoming connection from a client.
Handles non-blocking accept and filters out temporary errors.
 Returns client file descriptor on success, -1 on error
 */
static int acceptConnection(int server_fd)
{
    struct sockaddr_in cli;
    socklen_t clilen = sizeof(cli);

    int cfd = accept(server_fd, (struct sockaddr*)&cli, &clilen);

    if (cfd < 0)
    {
        if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
            std::cerr << "accept() failed" << std::endl;
        return -1;
    }

    DEBUG_PRINT("New connection accepted (fd: " << RED << cfd << RESET << ")");
    return cfd;
}

/*
 Processes a single client connection by accepting it and handling the request.
 Closes the connection after handling.

 TODO: Replace with non-blocking I/O and persistent connections.
 Implement Client class.
 */
static void processConnection(HttpServer* server, int server_fd)
{
    int cfd = acceptConnection(server_fd);
    if (cfd < 0)
        return;

    server->handleClient(cfd);
    close(cfd);
}

/*
 Determines whether the server should continue serving based on serve-once mode.
 In serve-once mode, stops after serving one connection.

This function is used to control the behavior of the server in serve-once mode.
 */
static bool shouldContinueServing(bool serveOnce, size_t servedCount)
{
    if (!serveOnce)
        return true;

    return servedCount < 1;
}

/* Runs the main accept loop for the HTTP server.
 Listens for incoming connections and processes them.
  Can run in serve-once mode or continuous mode.
 */
int HttpServer::runAcceptLoop(int server_fd, bool serveOnce)
{
    size_t servedCount = 0;
    
    while (!g_stop)
    {
        int ready = waitForConnection(server_fd);
        
        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }

        if (ready == 0)
            continue; // timeout

        processConnection(this, server_fd);

        if (serveOnce)
        {
            ++servedCount;
            if (!shouldContinueServing(serveOnce, servedCount))
                break;
        }
    }

    DEBUG_PRINT("HTTP Server shutting down...");
    return 0;
}