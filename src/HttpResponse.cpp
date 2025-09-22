#include "HttpResponse.hpp"
#include "HTTPHeaders.hpp"
#include "HTTPparser.hpp"
#include "ConfigParser.hpp"


Response::Response()
{
    _targetfile = "";
    _response_headers = "";
    _response_body = "";
    _code = 0;
    _response_final = " ";
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
    _response_headers.append("Content-Length: ");
    _response_headers.append(std::to_string(_response_body.size()+_response_headers.size()) + "\r\n");
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

void Response::connection()
{
    if(request.getHeaders().getConnection() == "keep-alive")
        _response_headers.append(" Connection: keep-alive\r\n");
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

std::bool fileExits(const std::string& name)
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

void Response::buildResponse()
{
    if(request.getMethod() == "GET" && _code == 200)
    {
        _response_final = _response_headers + "\r\n"; 
    }
}

/*void Response::setHeaders()
{
    statusLine();
    appDate();
    appContentLen();
    appContentType();
    connection();
    server();

}*/

Response::~Response()
{

}