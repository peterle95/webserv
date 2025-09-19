#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

#include <string>

class Response
{
    private:
    std::string _file;
    std::string _response_body;
    std::string _response;
    public:
    //Constructor
    Response();
    //Response()
    ~Response();
    std::string body;
    std::string response_body;
    std::string response_headers;

    void appDate();
    void appContentType();
    void appContentLen();
    void appBody();
    void buildResponse();
    void setHeaders();
    void connection();
    void server();
};

#endif