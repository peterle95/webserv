#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_REQUEST_SIZE 2047
class Client
{
    private:
    socklen_t address_len;
    struct sockaddr_storage address;
    int socket;
    char request[MAX_REQUEST_SIZE+1];
    struct client_info *next;

    Client();
    struct sockaddr_storage client_address;
    socklen_t client_len;
    ~Client();
    //SOCKET ;

    void getClientInfo();
    std::string getClient();
    void dropClient();

};
#endif