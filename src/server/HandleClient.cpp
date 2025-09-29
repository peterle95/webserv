#include "Common.hpp"

// === INTEGRATION POINT ===
 // Replace `handleClient()` with Client class to implement
 // non-blocking, stateful per-connection handling (read/parse/write, keep-alive).
 // Integrate the Http Response logic
 // Integrate the Http Request parsing logic
 // HTTP 1.1 requires to support keep-alive
 // Client timouts
 // 
 void HttpServer::handleClient(int client_fd)
 {
     std::string request;
     char buf[4096];
 
     // Read request headers (blocking)
     while (true)
     {
         ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
         if (n > 0)
         {
             request.append(buf, buf + n);
             if (request.find("\r\n\r\n") != std::string::npos)
                 break;
             continue;
         }
         if (n == 0)
             break; // peer closed
         if (n < 0)
         {
             if (errno == EINTR) continue;
             if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
             return; // fatal error
         }
     }
 
     HTTPparser parser;
     parser.parseRequest(request);
     bool keepAlive = determineKeepAlive(parser);
 
     std::string response;
     if (!parser.isValid()) // integrate here the Http Response logic
     {
         DEBUG_PRINT(RED << "Request parsing failed: " << parser.getErrorMessage() << RESET);
         response = generateBadRequestResponse(keepAlive);
     }
     else if (parser.getMethod() == "GET")
     {
         DEBUG_PRINT("Valid request parsed:");
         DEBUG_PRINT("   Method: " << RED << parser.getMethod() << RESET);
         DEBUG_PRINT("   Path: " << RED << parser.getPath() << RESET);
         DEBUG_PRINT("   Version: " << RED << parser.getVersion() << RESET);
         response = generateGetResponse(parser.getPath(), keepAlive);
     }
     else
     {
         DEBUG_PRINT("Method not allowed: " << RED << parser.getMethod() << RESET << " (only GET supported)");
         response = generateMethodNotAllowedResponse(keepAlive);
     }
 
     // Send full response (blocking)
     size_t off = 0;
     while (off < response.size())
     {
         ssize_t sent = send(client_fd, response.c_str() + off, response.size() - off, 0);
         if (sent > 0) { off += static_cast<size_t>(sent); continue; }
         if (sent < 0 && errno == EINTR) continue;
         if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
         break; // other error
     }
 
     // NOTE: Keep-alive not supported yet
 }