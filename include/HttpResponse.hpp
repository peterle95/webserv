#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

/*#include <string>
#include "HTTPparser.hpp"
#include "ConfigParser.hpp"
#include "HttpServer.hpp"*/

#include "Common.hpp"
/*#include "Cgi.hpp"
#include "HttpResponse.hpp"
#include "HTTPHeaders.hpp"
#include "HTTPparser.hpp"
#include "HTTPRequestLine.hpp"
#include "ConfigParser.hpp"
#include "HttpServer.hpp"
#include <Cgi.hpp>
#include <map>
#include <string>
#include <sstream>

#include <fstream>
#include <iostream>
#include <cstdlib>
#include "Client.hpp"*/

class HttpServer;
class ConfigParser;


class Response
{
    private:
    std::string _targetfile;
    std::string _response_body;
    std::string _response;
    std::string body;
    std::string _response_headers;
    std::string _response_final;
    std::string _loc;
    std::string _request;
    int _code;
    std::string root;
    std::string _path;
    int _ServerIndex;
    HttpServer *_HttpServer;
    HTTPparser _HttpParser;
    public:
    //Response();
    
    //HTTPparser _HTTPParser;
    //ConfigParser _ConfigParser;
    
    Response( int ServerIndex ,HttpServer *server ,  HTTPparser &HttpParser,ConfigParser &ConfigParser);
    void setRequest(std::string request);
    ~Response();
    HTTPparser request;
    ConfigParser _ConfigParser;
    void appDate();
    void appContentType();
    void appContentLen();
    int appBody();
    void buildResponse();
    void setHeaders();
    void connection();
    //void server();
    void builderror_body(int code);
    void builderror_responses(int code);
    bool isDirectory(std::string path);
    bool fileExits(const std::string& name);
    std::string appRoot(std::string _path, std::string _target);
    int reqErr();
    std::string statusMessage(int code);
    void statusLine();
    bool fileExists(const std::string& name);
    //int buildResponse();
    //int _cgi;
    void server();
    //int buildResponse();
    //void appBody();
    std::string buildErrorPage(int code);
    std::string getResponse();
    std::string redirecUtil();
    const Response &operator=(const Response& other);
};

#endif
