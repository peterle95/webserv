#include "Common.hpp"
#include "HttpServer.hpp"

static int waitForConnection(int server_fd)
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);
    
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    
    int ready = select(server_fd + 1, &read_fds, NULL, NULL, &tv);
    if (ready < 0 && errno != EINTR)
        std::cerr << "select() on server socket failed" << std::endl;
    
    return ready;
}

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

static void processConnection(HttpServer* server, int server_fd)
{
    int cfd = acceptConnection(server_fd);
    if (cfd < 0)
        return;
    
    server->handleClient(cfd);
    close(cfd);
}

static bool shouldContinueServing(bool serveOnce, size_t servedCount)
{
    if (!serveOnce)
        return true;
    
    return servedCount < 1;
}

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