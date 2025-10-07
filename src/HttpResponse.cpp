#include "HttpResponse.hpp"
#include "HTTPHeaders.hpp"
#include "HTTPparser.hpp"
#include "ConfigParser.hpp"
#include <cgi.h>


Response::Response()
{
    _targetfile = "";
    _response_headers = "";
    _response_body = "";
    _code = 0;
    _response_final = " ";
    _loc = "";
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

    if(respone.body.empty())
        _response_headers.append("Content-Length: 0\r\n");
    else
    _response_headers.append("Content-Length: ");
    _response_headers.append(std::to_string(_response_body.size()) + "\r\n");
}

void Response::appContentType()
{
    std::unordered_map<std::string, std::string> mime_types =
    {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".txt", "text/plain"},
        {".pdf", "application/pdf"},
    };
    //location_config loc;
    std::string targetfile = loc.getPath();
    size_t dot_pos = _targetfile.rfind('.');
    if (dot_pos != std::string::npos)
    {
        std::string fileExt = _targetfile.substr(dot_pos + 1);
        auto it = mimeTypes.find(fileExt);
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

void builderror_body(int code)
{
    if(code == 404)
    _response_body.append(" Not Found");
    else if(code == 413)
    _response_body.append(" Payload Too Large");
}

void Response::appBody()
{
 if(request.getMethod() == "GET" && _code == 200)
 {
    std::ifstream file(_targetfile);
    if(file)
    {
        std::ostringstream ss;
        ss << file.rdbuf();
        _response_body = ss.str();
        file.close();
    }
}
 else
    builderror_body(_code);
}

void Response::statusLine()
{
    _response_headers.append("HTTP/1.1 ");
    _response_headers.append(std::tostring(_code));
    if(code == 200)
        _response_headers.append(" OK\r\n");
    else if(code == 404)
        _response_headers.append(" Not Found\r\n");
    else if(code == 500)
        _response_headers.append(" Internal Server Error\r\n");
    else if(code == 400)
        _response_headers.append(" Bad Request\r\n");
    else if(code == 403)
        _response_headers.append(" Forbidden\r\n");
    else if(code == 301)
        _response_headers.append(" Moved Permanently\r\n");
    else if(code == 302)
        _response_headers.append(" Found\r\n");
    else if(code == 405)
        _response_headers.append(" Method Not Allowed\r\n");
    else
        _response_headers.append(" Unknown Status\r\n");
}

std::string statusMessage(int code)
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
    if(request.getHeaders().getConnection() == "keep-alive")
        _response_headers.append(" Connection: keep-alive\r\n");
    else
        _response_headers.append(" Connection: close\r\n");
}

void Response::server()
{
 HTTPHeaders header;
 _response_headers.append(" Server: ");
 _response_headers.append("Webserv/1.1")
 _response_headers.append("\r\n");
}

std::bool isDirectory(std::string path)
{
    struct stat path_stat;
    stat(path.c_str(), &path_stat);
    return S_ISDIR(path_stat.st_mode);
}


std::bool fileExists(const std::string& name)
{
    ifstream f(name.c_str());
    return f.good();
}

std::string Response::appRoot(std::string _path, std::string _target)
{
    //if(request.path == )
    
    if(!isDirectory(_path))
    {
        _code = 404;
        return _target = " ";

    }
    else if(!isFile(_path))
    {
        _code = 404;
        return _target = " ";
    }
    else 
    return server.getRoot() + request.getPath();
}

int Response::appBody()
{
 if(request.getMethod() == "GET")
 {
    std::ifstream file(_targetfile);
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
else if(request.getMethod() == "POST" &&  client_max_body_size < request.getBody().size())
{
    _code = 413;
    return 1;
}
else if(request.getMethod() == "POST" && request.getPath.find("cgi-bin") != std::string::npos)
{
    execute();
    _code = 200;
    _cgi = 1;
    readResponse();
    return 0;
}
else if(request.getMethod() == "DELETE" && request.getPath.find("cgi-bin") != std::string::npos)
{
    if(fileExists(_targetfile.c_str()))
    {
    execute();
    _code = 200;
    _cgi = 1;
    readResponse();
    return 0;
   }
   else
   {
    _code = 404;
    return 1;
   }
}
}


std::string Response::buildErrorPage(int code)
{
  return ("<html><head><title>" + std::to_string(_code) +" " + statusMessage(code) +" Error</title></head>"
          "<body><h1>" + std::to_string(_code) + " " + statusMessage(code) << "</h1>"
          "</body></html>");
}

void Response::builderror_body(int code)
{
    if(server.getErrorPage(code).empty() || !server.getErrorPage(code).count(code))
        buildErrorPage(code);
    else
    {
       std::string file = server.getroot() + server.getErrorPage().at(code);
       if(fileExists(file))
       {
         std::ostringstream ss;
        ss << file.rdbuf();
        _response_body = ss.str();
        file.close();
        //return 0;
       }
       else
         buildErrorPage(404);
    }    
}

int Response::buildResponse()
{
    if(reqErr() || appbody())
    {
        builderror_body(_code);
    }
    else if(_cgi)
    {
        return 0;
    }
    else if(request.getMethod() == "GET" && _code == 200)
    {
        _response_final = _response_headers + "\r\n"; 
    }
}

/*void Response::setHeaders()
=======
    //to do for pots and delete
}

void Response::setHeaders()
>>>>>>> c0886c7 (Resouce.cpp)
{
    statusLine();
    appDate();
    appContentLen();
    appContentType();
    connection();
    server();
<<<<<<< HEAD

}*/


void Response::buildResponse()
{
 appBody();
 setHeaders();
}

int Response::reqErr()
{
    if(request.getErrorStatusCode() != 0)
    {
        _code = request.getErrorStatusCode();
        return 1;
    }
}

Response::~Response()
{
}
{

}
