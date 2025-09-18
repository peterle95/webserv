#include "HttpResponse.hpp"
#include ""

Response::Response()
{
    _file = "";
    _response_body = "";
    _response = " ";
}

void Response::appDate()
{

}

void Response::appContentLen()
{

}

void Response::appContentType()
{
    _response.append("Content-Type: ");

}

void Response::appBody()
{

}

void Response::connection()
{
 //request.get
}

void Response::server()
{

}

void Response::buildResponse()
{

}

void Response::setHeaders()
{

}

Response::~Response()
{

}