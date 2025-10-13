/*#include "HttpResponse.hpp"
#include "HTTPHeaders.hpp"
#include "HTTPparser.hpp"
#include "HTTPRequestLine.hpp"
#include "ConfigParser.hpp"
#include "HttpServer.hpp"

#include <map>
#include <string>

#include <fstream>
#include <iostream>
#include <cstdlib>
#include "Client.hpp"*/
#include "Common.hpp"



/*Response::Response()
{
    _targetfile = "";
    _response_headers = "";
    _response_body = "";
    _code = 0;
    _response_final = " ";
    _loc = "";
}*/

Response::Response(HttpServer &HttpServer, ConfigParser &ConfigParser): _HttpServer(HttpServer), _ConfigParser(ConfigParser)
{   
   // _ConfigParser = ConfigParser;
    //_HTTPParser = HTTPparser;
    //_HttpServer = HttpServer;
    //_Cgi = cgi;
    //_cgi = 0;
    _targetfile = "";
    _response_headers = "";
    _response_body = "";
    _code = 0;
    _response_final = " ";
    _loc = "";
    _targetfile = _HTTPParser.getCurrentFilePath();
}


void Response::appDate()
{
    time_t now = time(0);
    char HTTPDate[100];
    strftime(HTTPDate, sizeof(HTTPDate), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    _response_headers.append(" Date: ");
    _response_headers.append(HTTPDate);
    _response_headers.append("\r\n");
}

void Response::appContentLen()
{
    std::stringstream ss;
    int size;
    _response_headers.append("Content-Length: ");
    size = _response_body.size() + _response_headers.size();
    ss << size;
    _response_headers.append((ss.str()) + "\r\n");
    if(_response_body.empty())
        _response_headers.append("Content-Length: 0\r\n");
    else
    _response_headers.append("Content-Length: ");
    ss << _response_body.size();
    _response_headers.append((ss.str() + "\r\n"));

}

void Response::appContentType()
{
    std::map<std::string, std::string> mime_types;
    
        mime_types.insert((std::make_pair(".html", "text/html")));
        mime_types.insert((std::make_pair(".htm", "text/html")));
        mime_types.insert((std::make_pair(".css", "text/css")));
        mime_types.insert((std::make_pair(".js", "application/javascript")));
        mime_types.insert((std::make_pair(".json", "application/json")));
        mime_types.insert((std::make_pair(".png", "image/png")));
        mime_types.insert((std::make_pair(".jpg", "image/jpeg")));
        mime_types.insert((std::make_pair(".jpeg", "image/jpeg")));
        mime_types.insert((std::make_pair(".gif", "image/gif")));
        mime_types.insert((std::make_pair(".txt", "text/plain")));
        mime_types.insert((std::make_pair(".pdf", "application/pdf")));
    
    size_t dot_pos = _targetfile.rfind('.');
    if (dot_pos != std::string::npos)
    {
        std::string fileExt = _targetfile.substr(dot_pos + 1);
        std::map<std::string, std::string>::iterator it = mime_types.find(fileExt);
        if (it != mime_types.end())
        {
            _response_headers.append(" Content-Type: " + mime_types[fileExt] + "\r\n");
        }
        else
        {
            _response_headers.append(" Content-Type: text/html r\n");
        }
    }   
}

void Response::builderror_body(int code)
{
    if(code == 404)
    _response_body.append(" Not Found");
     else if(code == 413)
    _response_body.append(" Payload Too Large");
}

void Response::statusLine()
{
    _response_headers.append("HTTP/1.1 ");
    //ss.str();
    std::ostringstream oss;
    oss << _code;
    _response_headers.append(oss.str());
    if(_code == 200)
        _response_headers.append(" OK\r\n");
    else if(_code == 404)
        _response_headers.append(" Not Found\r\n");
    else if(_code == 500)
        _response_headers.append(" Internal Server Error\r\n");
    else if(_code == 400)
        _response_headers.append(" Bad Request\r\n");
    else if(_code == 403)
        _response_headers.append(" Forbidden\r\n");
    else if(_code == 301)
        _response_headers.append(" Moved Permanently\r\n");
    else if(_code == 302)
        _response_headers.append(" Found\r\n");
    else if(_code == 405)
        _response_headers.append(" Method Not Allowed\r\n");
    else
        _response_headers.append(" Unknown Status\r\n");
}

std::string Response::statusMessage(int code)
{
    if(code == 200)
        return "OK";
    else if(code == 404)
        return "Not Found";
    else if(code == 500)
        return "Internal Server Error";
    else if(code == 400)
        return "Bad Request";
    else if(code == 403)
        return "Forbidden";
    else if(code == 301)
        return "Moved Permanently";
    else if(code == 302)
        return "Found";
    else if(code == 405)
        return "Method Not Allowed";
    else
        return "Unknown Status";
}

void Response::connection()
{
    if(_HttpServer.determineKeepAlive(_HTTPParser))
        _response_headers.append(" Connection: keep-alive\r\n");
    else
        _response_headers.append(" Connection: close\r\n");
}

void Response::server()
{
 _response_headers.append(" Server: ");
 _response_headers.append("Webserv/1.1");
 _response_headers.append("\r\n");
}

bool Response::isDirectory(std::string path)
{
    struct stat path_stat;
    stat(path.c_str(), &path_stat);
    return S_ISDIR(path_stat.st_mode);
}


bool Response::fileExists(const std::string& name)
{
    std::ifstream f(name.c_str());
    return f.good();
}



/*std::string Response::appRoot(std::string _path, std::string _target)
{
    //if(request.path == )
    
    if(!isDirectory(_path))
    {
        _code = 404;
        return _target = " ";

    }
    else if(!fileExists(_path))
    {
        _code = 404;
        return _target = " ";
    }
    else 
    return (_server.getRoot() + request.getPath());
}*/

//void Response::buildResponse()
//{

int Response::appBody()
{
const LocationConfig *currentLocation =_HttpServer.getCurrentLocation();
std::string _targetfile = _HttpServer.getFilePath(currentLocation->path);
 if(request.getMethod() == "GET")
 {
    std::ifstream file(_targetfile.c_str());
    if(file)
    {
        std::ostringstream ss;
        ss << file.rdbuf();
        _response_body = ss.str();
        file.close();
        _code = 200;
        return 0;
    }
    _code = 413;
    return 1;
}
else if(request.getMethod() == "POST" &&  _ConfigParser.getClientMaxBodySize() < request.getBody().size())
{
    _code = 413;
    return 1;
}
else if(request.getMethod() == "POST" && request.getPath().find("cgi-bin") != std::string::npos)
{
    if(currentLocation->cgiPass == true && !currentLocation->cgiExtension.empty())
    {
     std::string cgiResponse;   
     cgiResponse =_HttpServer.processCGI(request);
     if(!cgiResponse.empty())
     {
        _response_body = cgiResponse;
        _code = 200;
        return 0;
     }
     else
     {
        _code = 500;
        return 1;
     }
    return 0;
    }
}
else if(request.getMethod() == "DELETE" && request.getPath().find("cgi-bin") != std::string::npos)
{
    if(fileExists(_targetfile.c_str()) && currentLocation->cgiPass == true && !currentLocation->cgiExtension.empty())
    {
        std::string cgiResponse;
        cgiResponse =_HttpServer.processCGI(request);
        if(!cgiResponse.empty() && remove(_targetfile.c_str()) != 0)
        {
            _code = 404; //Resource doesnt exist
            return 1;
        }
        else
        {
            _code = 204; //No Content
            return 0;
        }
    return 0;
   }
   else
   {
    _code = 404;
    return 1;
   }
}
return 1;
}

std::string Response::buildErrorPage(int code)
{
    std::ostringstream ss;
    ss <<  code;
    _code = code;
  return ("<html><head><title>" + (ss.str()) +" " + statusMessage(code) +" Error</title></head>"
          "<body><h1>" + (ss.str()) + " " + statusMessage(code) + "" + "</h1>"
          "</body></html>");
}

void Response::builderror_responses(int code)
{
    std::string _errorPage =_ConfigParser.getErrorPage(code);
    
    if(_errorPage.empty())
        buildErrorPage(code);
    else
    {
       std::string path = _ConfigParser.getRoot() + _ConfigParser.getErrorPage(code).at(code);
       if(fileExists(path))
       {
         std::ifstream file(path.c_str());
         std::stringstream buffer;
         buffer << file.rdbuf();
        _response_body = buffer.str();
        file.close();
       }
       else
         buildErrorPage(404);
    }    
}

void Response::buildResponse()
{
    if(reqErr() || appBody())
    {
        builderror_body(_code);
    }
   /* else if(_cgi)
    {
        return;
    }*/

    if(request.getMethod() == "GET" && _code == 200)
    {
        _response_final = _response_headers + "\r\n"; 
    }
}

void Response::setHeaders()
{
    statusLine();
    appDate();
    appContentLen();
    appContentType();
    connection();
    //server();
}


int Response::reqErr()
{
    if(!request.getErrorStatusCode().empty())
    {
        _code = std::atoi(request.getErrorStatusCode().c_str());
        return _code;
    }
    return 0;
}

Response::~Response()
{
}
