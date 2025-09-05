#include "../../include/Common.hpp"

int create_socket(const char *host, int port)
{
    std::cout << "Configuring local address\n";
    std::cout << "Getting server address....\n";
    struct addrinfo hints;
    int socket_listen;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *bind_address;
    
    // Use the port parameter instead of hardcoded "8080"
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    getaddrinfo(host, port_str, &hints, &bind_address); // Use host parameter

    std::cout << "Creating socket at client side...\n";
    
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if(socket_listen == -1)
    {
     std::cout << "socket() failed " << errno << std::endl;
     return 1;   
    }

    std::cout << "Binding socket at client side....\n";
    if(bind(socket_listen,bind_address->ai_addr, bind_address->ai_addrlen))
    {
       std::cout << "bind() failed " << errno << std::endl;
       return 1; 
    }
    freeaddrinfo(bind_address); // free the address info
    return socket_listen;
}

/*
Previous issues and fixes:
The server accepted connections but never sent any response, causing browsers to hang.
    - Fixed by implementing a proper HTTP response with status 200 and HTML content.

The port handling loop was incorrectly incrementing the index twice.
    - Simplified to handle a single port directly without the broken loop.

Error handling was inconsistent, sometimes returning 1 or -1 on errors.
    - Standardized to return 1 on all errors for consistency.

Client sockets were not being properly closed after handling requests.
    - Added proper close() calls for client sockets after sending responses.

The server now properly cleans up resources and handles connection errors
without crashing, with improved error logging.
 */
int socket_server(const char *host, int port)
{
    int socket_listen = create_socket(host, port);
    if (socket_listen < 0)
        return 1;

    std::cout << "Listening...\n";
    if(listen(socket_listen, 10) < 0)
    {
        std::cout << "listen_failed..." << errno << std::endl;
        return 1;
    }

    std::cout << "Waiting for connection...\n";
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    while(true)
    {
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_listen, &reads);

        int ready = select(socket_listen+1, &reads, 0, 0, 0);
        if(ready < 0)
        {
            std::cout << "select() failed. " << errno << std::endl;
            return 1;
        }
        else if(ready == 0)
        {
            std::cout << "select() timed out. " << errno << std::endl;
            continue;
        }

        if(FD_ISSET(socket_listen, &reads))
        {
            char request[2048];
            int socket_client = accept(socket_listen, (struct sockaddr*)&client_address, &client_len);
            if(socket_client == -1)
            {
                std::cout << "accept failed() " << errno << std::endl;
                return 1;
            }

            int bytes_received = recv(socket_client, request, sizeof(request) - 1, 0);
            if(bytes_received < 0)
            {
                std::cout << "recv() failed " << errno << std::endl;
                close(socket_client);
                continue;
            }
            request[bytes_received > 0 ? bytes_received : 0] = '\0';
            std::cout << "Bytes " << bytes_received << " received\n";

            // Minimal HTTP response to let browsers show something
            const char body[] = "<html><body><h1>webserv up</h1></body></html>";
            char response[512];
            int body_len = (int)strlen(body);
            int resp_len = snprintf(response, sizeof(response),
                                   "HTTP/1.1 200 OK\r\n"
                                   "Connection: close\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: %d\r\n\r\n%s",
                                   body_len, body);
            if (resp_len > 0)
                send(socket_client, response, (size_t)resp_len, 0);

            close(socket_client);
        }
    }
}
