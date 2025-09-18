#include "Client.hpp"

#include <iostream>
#include <netdb.h>

void Client::getClientInfo()
{
  std::cout << "Client is connected" << std::endl;
  char address_buffer[100];
  getnameinfo((struct sockaddr *)&client_address, client_len, address_buffer, sizeof(address_buffer), 0,0, NI_NUMERICHOST);
  std::cout << " Address buffer " << address_buffer << std::endl;
}

std::string Client::getClient()
{

}

void Client::dropClient()
{

}