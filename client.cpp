#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main()
{
    /*if (argc < 3) {
    fprintf(stderr, "usage: tcp_client hostname port\n");
    return 1;
    }*/
    printf("Getting remote address....\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *peer_address;
    getaddrinfo("127.0.0.1", "8080", &hints, &peer_address);

    printf("Creating socket at client side...\n");
    

    /*printf("Binding socket at client side....\n");
    if(bind(socket_peer ,peer_address->ai_addr, peer_address->ai_addrlen))
    {
       fprintf(stderr, "bind() failed. %d\n ", errno);
       return 1; 
    }*/
    
    while(true)
    {
    int socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);
    if(socket_peer == -1)
    {
     fprintf(stderr, "socket() failed. %d\n", errno);
     return 1;   
    }
    printf("Connecting to server....\n");
    if(connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen))
    {
        fprintf(stderr, "connection failed(). %d\n", errno);
        return 1;
    }
    printf("To send data, enter text followed by enter.\n");
    printf("Sending response...\n");
    const char *response = 
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Content-Type: text/plain\r\n\r\n";
    int bytes_sent = send(socket_peer, response, strlen(response), 0);
    printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));
    }
    freeaddrinfo(peer_address);


}
