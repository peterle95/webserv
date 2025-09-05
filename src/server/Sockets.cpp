#include "../../include/Common.hpp"

int create_socket(const char *host, int port)
{
    printf("Configuring local address\n");
    printf("Getting server address....\n");
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

    printf("Creating socket at client side...\n");
    
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if(socket_listen == -1)
    {
     fprintf(stderr, "socket() failed %d ", errno);
     return 1;   
    }

    printf("Binding socket at client side....\n");
    if(bind(socket_listen,bind_address->ai_addr, bind_address->ai_addrlen))
    {
       fprintf(stderr, "bind() failed %d", errno);
       return 1; 
    }
    freeaddrinfo(bind_address); // free the address info
    return socket_listen;
}

int socket_server(const char *host, int port)
{
    int socket_listen;
    int ports[] = {port};
    int num_ports = sizeof(ports)/sizeof(ports[0]);
    for(int i = 0; i < num_ports ; i++)
    {
        socket_listen = create_socket(host, ports[i]);
        i++;
    }
    printf("listening....\n");
    if(listen(socket_listen, 10) < 0)
    {
        fprintf(stderr, "listen_failed...%d\n", errno);
        return 1;
    }

    printf("Waiting for connection...\n");
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    while(true)
    {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(socket_listen, &reads);
    printf("Client is connected...\n");
    int ready = select(socket_listen+1, &reads, 0, 0, 0);
    if(ready < 0)
    {
        fprintf(stderr,"select() failed. %d\n", errno);
        return 1;
    }
    else if(ready == 0)
    {
        fprintf(stderr,"select() timed out. %d\n", errno);
        return 1;
    }
    else
    {
        if(FD_ISSET(socket_listen, &reads))
        {
        char request[1024];
        int socket_client; 
        socket_client = accept(socket_listen, (struct sockaddr*)&client_address, &client_len);
        if(socket_client == -1)
        {
        fprintf(stderr, "accept failed() %d", errno);
        return 1;
        }
        int bytes_received = recv(socket_client, request, 1024, 0);
        if(bytes_received < 0)
        {
            printf("bytes received is less\n");
        }
         printf("Bytes %d received ", bytes_received);
        }
    }
    }
   
    /*char address_buffer[100];
    getnameinfo((struct sockaddr*)&client_address, client_len, address_buffer ,sizeof(address_buffer), 0,0, NI_NUMERICHOST);
    printf("%s/n", address_buffer);*/
}
