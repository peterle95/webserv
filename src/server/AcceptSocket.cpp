#include "Common.hpp"

int HttpServer::runAcceptLoop(int server_fd, bool serveOnce)
 {
     size_t servedCount = 0;
 
     while (!g_stop)
     {
         fd_set read_fds;
         FD_ZERO(&read_fds);
         FD_SET(server_fd, &read_fds);
         struct timeval tv; tv.tv_sec = 1; tv.tv_usec = 0;
         int ready = select(server_fd + 1, &read_fds, NULL, NULL, &tv);
         if (ready < 0)
         {
             if (errno == EINTR) continue;
             std::cerr << "select() on server socket failed" << std::endl;
             break;
         }
         if (ready == 0)
             continue; // timeout
 
         struct sockaddr_in cli;
         socklen_t clilen = sizeof(cli);
         int cfd = accept(server_fd, (struct sockaddr*)&cli, &clilen);
         if (cfd < 0)
         {
             if (errno == EINTR) continue;
             if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
             std::cerr << "accept() failed" << std::endl;
             continue;
         }
 
         DEBUG_PRINT("New connection accepted (fd: " << RED << cfd << RESET << ")");
 
         handleClient(cfd);
         close(cfd);
 
         if (serveOnce)
         {
             ++servedCount;
             if (servedCount >= 1) break;
         }
     }
 
     DEBUG_PRINT("HTTP Server shutting down...");
     return 0;
 }