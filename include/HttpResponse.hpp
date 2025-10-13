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
    int _code;
    HTTPparser _HTTPParser;
    HttpServer _HttpServer;
    ConfigParser _ConfigParser;
    std::string root;
    public:
    //Response();
    Response(HttpServer &server, ConfigParser &configParser);
    ~Response();
    HTTPparser request;
    //ConfigParser configParser;
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
    std::string getResponse() { return _response_final; }
};

#endif
