#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

#include <string>

class Response
{
    private:
    std::string _file;
    std::string _response_body;
    std::string _response;
    std::string body;
    std::string response_body;
    std::string response_headers;
    std::string _response_final;
    int _code;
    
    std::string root;
    public:
    //Constructor
    Response();
    //Response()
    ~Response();
    HTTPparser request;
    ConfigParser server;
    void appDate();
    void appContentType();
    void appContentLen();
    void appBody();
    void buildResponse();
    void setHeaders();
    void connection();
    void server();
    void builderror_body(int code);
    std::bool isDirectory(std::string path);
    std::bool fileExits(const std::string& name);
    std::string appRoot(std::string _path, std::string _target);
<<<<<<< HEAD
=======
    int reqErr();
    std::string statusMessage(int code);
    int buildResponse();
    int _cgi;
>>>>>>> c0886c7 (Resouce.cpp)
    
};

#endif