#include "Common.hpp"

int HttpServer::createAndBindSocket()
{
     int server_fd = socket(AF_INET, SOCK_STREAM, 0);
     if (server_fd < 0)
     {
         std::cerr << "socket() failed" << std::endl;
         return -1;
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
         return -1;
     }
 
     if (listen(server_fd, 128) < 0)
     {
         std::cerr << "listen() failed" << std::endl;
         close(server_fd);
         return -1;
     }
 
     if (!HttpServer::setNonBlocking(server_fd))
     {
         std::cerr << "Failed to set server socket non-blocking" << std::endl;
         close(server_fd);
         return -1;
     }
 
     return server_fd;
 }