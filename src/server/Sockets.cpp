#include "../../include/Common.hpp"

// Changed function signature to take host and port parameters for flexibility
// (previously hardcoded to 0.0.0.0:8080)
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
    
    // Made port configurable instead of hardcoded "8080"
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    getaddrinfo(host, port_str, &hints, &bind_address);

    std::cout << "Creating socket at client side...\n";
    
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if(socket_listen == -1)
    {
        std::cout << "socket() failed " << errno << std::endl;  // Changed to std::cout for consistency
        return -1;   // Changed to -1 for better error handling
    }

    std::cout << "Binding server socket....\n";  // Fixed misleading message (was "client side")
    if(bind(socket_listen,bind_address->ai_addr, bind_address->ai_addrlen))
    {
        std::cout << "bind() failed " << errno << std::endl;  // Changed to std::cout
        return -1;  // Changed to -1 for consistency
    }
    freeaddrinfo(bind_address);  // Added missing memory cleanup
    return socket_listen;
}

// Changed from main() to socket_server() for better code organization
int socket_server(const char *host, int port)
{
    // Removed array loop that was causing double increment (i++ in both for and loop body)
    int socket_listen = create_socket(host, port);
    if (socket_listen < 0)
        return -1;

    std::cout << "Listening...\n";
    if(listen(socket_listen, 10) < 0)
    {
        std::cout << "listen() failed... " << errno << std::endl;  // Improved error message
        return -1;
    }

    std::cout << "Waiting for connections...\n";
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    while(true)
    {
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_listen, &reads);

        int ready = select(socket_listen + 1, &reads, 0, 0, 0);  // Added spaces for readability
        if(ready < 0)
        {
            std::cout << "select() failed. " << errno << std::endl;  // Consistent error reporting
            return -1;
        }
        else if(ready == 0)
        {
            std::cout << "select() timed out. " << errno << std::endl;
            continue;  // Changed from return to continue
        }
        request[bytes_received > 0 ? bytes_received : 0] = '\0';
        std::cout << "Bytes " << bytes_received << " received\n";

        // Minimal HTTP response to let browsers show something
        // T
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

