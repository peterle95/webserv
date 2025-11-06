#include "Common.hpp"

Response::Response(int ServerIndex, HttpServer &HttpServer, HTTPparser &HTTPParser, ConfigParser &ConfigParser) : _ServerIndex(ServerIndex), _HttpServer(HttpServer), _HttpParser(HTTPParser), _ConfigParser(ConfigParser)
{
    _request = "";
    _targetfile = "";
    _response_headers = "";
    _response_body = "";
    _code = 0;
    _response_final = " ";
    _loc = "";
}

const Response& Response::operator=(const Response& other) {
        if (this != &other) {
            // Properly copy all members, especially std::string members
            _loc = other._loc;
            _targetfile = other._targetfile;
            _response_body = other._response_body;
            _response = other._response;
            body = other.body;
            _response_headers = other._response_headers;
            _response_final = other._response_final;
            _request = other._request;
            _code = other._code;
            root = other.root;
            _HttpServer = other._HttpServer;
            _HttpParser = other._HttpParser;
            _ConfigParser = other._ConfigParser;
            _ServerIndex = other._ServerIndex;
}
        return *this;
    }

void Response::setRequest(std::string request)
{
 this->_request = request;
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
    size = _response_body.size();
    ss << size;
   // _response_headers.append((ss.str()) + "\r\n");
    if (_response_body.empty())
        _response_headers.append("Content-Length: 0");
    else
    {
        _response_headers.append("Content-Length: ");
        _response_headers.append(ss.str());
    }
    _response_headers.append("\r\n");
}

// Sets the Content-Type header based on the file extension of the target file.
void Response::appContentType()
{
    // Map of file extensions to MIME types
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

    // Find the last dot in the target file name to get the extension
    size_t dot_pos = _targetfile.rfind('.');
    if (dot_pos != std::string::npos)
    {
        // Extract the file extension including the dot
        std::string fileExt = _targetfile.substr(dot_pos);
        // Look up the MIME type for the extension
        std::map<std::string, std::string>::iterator it = mime_types.find(fileExt);
        if (it != mime_types.end())
        {
            // If found, set the Content-Type header accordingly.
            // For text-like mime types include a charset to ensure proper display of UTF-8 characters (emoji, accents, etc.).
            std::string mime = it->second;
            bool is_text = (mime.find("text/") == 0) || (mime == "application/javascript") || (mime == "application/json") || (mime == "application/xml");
            if (is_text)
                mime += "; charset=utf-8";
            _response_headers.append("Content-Type: " + mime + "\r\n");
        }
        else
        {
            // Default to text/html (with UTF-8 charset) if extension is unknown
            _response_headers.append("Content-Type: text/html; charset=utf-8\r\n");
        }
    }
}

// Generates a simple error message in the response body based on the provided HTTP status code.
// Supported codes:
void Response::builderror_body(int code)
{
    std::ostringstream ss;
    ss << code;
    _response_body.append(ss.str());
    if (code == 404)
        _response_body.append(" Not Found");
    else if (code == 413)
        _response_body.append(" Payload Too Large");
}

void Response::statusLine()
{
    _response_headers.append("HTTP/1.1 ");
    // ss.str();
    std::ostringstream oss;
    oss << _code;
    _response_headers.append(oss.str());
    if (_code == 200)
        _response_headers.append(" OK\r\n");
    else if (_code == 404)
        _response_headers.append(" Not Found\r\n");
    else if (_code == 500)
        _response_headers.append(" Internal Server Error\r\n");
    else if (_code == 400)
        _response_headers.append(" Bad Request\r\n");
    else if (_code == 403)
        _response_headers.append(" Forbidden\r\n");
    else if (_code == 301)
        _response_headers.append(" Moved Permanently\r\n");
    else if (_code == 302)
        _response_headers.append(" Found\r\n");
    else if (_code == 405)
        _response_headers.append(" Method Not Allowed\r\n");
    else
        _response_headers.append(" Unknown Status\r\n");
}

std::string Response::statusMessage(int code)
{
    if (code == 200)
        return "OK";
    else if (code == 404)
        return "Not Found";
    else if (code == 500)
        return "Internal Server Error";
    else if (code == 400)
        return "Bad Request";
    else if (code == 403)
        return "Forbidden";
    else if (code == 301)
        return "Moved Permanently";
    else if (code == 302)
        return "Found";
    else if (code == 405)
        return "Method Not Allowed";
    else
        return "Unknown Status";
}

void Response::connection()
{
    if (_HttpServer.determineKeepAlive(_HttpParser) && _code == 200)
        _response_headers.append("Connection: keep-alive\r\n");
    else
        _response_headers.append("Connection: close\r\n");
}

void Response::server()
{
    _response_headers.append(" Server: ");
    _response_headers.append("Webserv/1.1");
    _response_headers.append("\r\n");
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

// void Response::buildResponse()

bool Response::isDirectory(std::string path)
{
    struct stat path_stat;
    if(stat(path.c_str(), &path_stat)!=0)
    {
        return false;
    }
     bool is_dir = S_ISDIR(path_stat.st_mode);
    std::cout << "📁 Is directory: " << (is_dir ? "YES" : "NO") << std::endl;
    
    return is_dir;
    /*if(stat(path.c_str(), &path_stat)!=0)
        return false;*/
    //return S_ISDIR(path_stat.st_mode);
}

int Response::appBody()
{
    const LocationConfig *currentLocation = _HttpServer.getCurrentLocation();
    _targetfile = _HttpParser.getCurrentFilePath();

    // Handle autoindex (directory listing) if enabled
    if(currentLocation->redirect.size() > 0)
    {
        _response_headers = redirecUtil();
        return 0;
    }
    else if ( isDirectory(_targetfile) && currentLocation->autoindex)
    {
        _response_body = generateDirectoryListing(_targetfile, _HttpParser.getPath());
        _code = 200;
        return 0;
    }
    else if (_request == "GET")
    {
        std::ifstream file(_targetfile.c_str());
        if (file.fail())
        {
            _code = 404;
            return 1;
        }
            std::ostringstream ss;
            ss << file.rdbuf();
            _response_body = ss.str();
            file.close();
            _code = 200;
            return 0;
    }
    else if (_request == "DELETE" || _request == "POST")
    {
        DEBUG_PRINT("Client Max Body Size: " << _HttpServer.getServerMaxBodySize(_ServerIndex) << ", Received Body Size: " << _HttpParser.getBody().size());
        if (_HttpServer.getServerMaxBodySize(_ServerIndex) < _HttpParser.getBody().size())
        {
            _code = 413;
            return 1;
        }
        else if (currentLocation->cgiPass == true && !currentLocation->cgiExtension.empty() && (_HttpServer.isMethodAllowed(_request)))
        {
            std::string cgiResponse;
            if(fileExists(_targetfile) == true)
            {
            cgiResponse = _HttpServer.processCGI(_HttpParser);
            _response_body = cgiResponse;
            if (!cgiResponse.empty())
            {
                //setHeaders();
               // _response_body = cgiResponse;
                //_response_final = _response_body +_response_headers ;
                _code = 200;
                return 0;
            }
            else
            {
                _code = 500;
                return 1;
            }
            }
            else if(fileExists(_targetfile) == false)
            {
            _code = 404;
            return 1;
          }
          /*else if(fileExists(_targetfile) == true && _HttpParser.getMethod() == "POST")
          {
            std::cout << "📄 Targetfile for POST .py: " << _targetfile << std::endl;
            std::ofstream outfile(_targetfile.c_str());
            outfile << _HttpParser.getBody();
            outfile.close();
            _code = 201;
            return 0;
          }*/
         else{
            // Method not allowed
            _code = 405;
            return 1;
        }
        }
        
    }
    else
    {
        _code = 404;
        return 1;
    }
  return 1;
}

void Response::buildErrorPage(int code)
{
    std::ostringstream ss;
    ss <<  code;
    _code = code;
    _response_final = ("<html><head><title>" + (ss.str()) +" " + statusMessage(code) +" Error</title></head>"
          "<body><h1>" + (ss.str()) + " " + statusMessage(code) + "" + "</h1>"
          "</body></html>");
}

int Response::stringToInt(const std::string &str)
{
    std::stringstream ss(str);
    int num;
    ss >> num;
    return num;
}

void Response::builderror_responses(int code)
{
    //(void)code;
    size_t _srvIndx = _HttpServer.selectServerForRequest(_HttpParser, stringToInt(_HttpParser.getServerPort()));//server based on host and port

    const std::string _errorPage = _ConfigParser.getServers().at(_srvIndx).getErrorPage(code);
    if (_errorPage.empty())
        buildErrorPage(code);
    else
    {
        std::string path = _ConfigParser.getServers().at(_srvIndx).getRoot() + _ConfigParser.getServers().at(_srvIndx).getErrorPage(code); //server based on host and port
        if (fileExists(path))
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

std::string Response::redirecUtil()
{
    std::string _response_headers;
    std::map<int, std::string> redirec = _HttpServer.getCurrentLocation()->redirect;
    std::map<int, std::string>::iterator it = redirec.begin();
    if (it != redirec.end())
    {
        // Build redirect response headers
        _response_headers.append("HTTP/1.1 ");
        // Create an output string stream to build the status code string
        std::ostringstream oss;
        oss << it->first;
        _response_headers.append(oss.str());
        std::string statusMsg = statusMessage(it->first);
        _response_headers.append(" " + statusMsg + "\r\n");
        _response_headers.append("Content-Length: 0\r\n");
        _response_headers.append("Location: " + it->second + "\r\n");
        _response_headers.append("Connection: close\r\n");
        _response_headers.append("\r\n");
    }
    return _response_headers;
}


void Response::buildResponse()
{
    _targetfile = _HttpParser.getCurrentFilePath();

    // Handle error responses or body generation
    if (reqErr() || appBody())
    {
        setHeaders();
        
        builderror_responses(_code);
        _response_final = _response_headers +_response_body;
        /*std::stringstream ss;
        ss << _response_body.size();

        // Build error body based on the response code
        
        _response_headers.append("HTTP/1.1 ");
        builderror_body(_code);
        std::string statusMsg = statusMessage(_code);
        _response_headers.append(" " + statusMsg + "\r\n");
        _response_headers.append("Content-Length: ");
        _response_headers.append(ss.str() + "\r\n");
        _response_headers.append("Connection: close\r\n");
        _response_headers.append("\r\n");
        _response_final = _response_headers + _response_body;*/
    }
    /*else if (!_HttpServer->getCurrentLocation()->redirect.empty()) // Handle HTTP redirects if configured
    {
        std::string _response_headers;
        std::map<int, std::string> redirec = _HttpServer->getCurrentLocation()->redirect;
        std::map<int, std::string>::iterator it = redirec.begin();
        if (it != redirec.end())
        {
            // Build redirect response headers
            _response_headers.append(" HTTP/1.1 ");
            std::ostringstream oss;
            oss << it->first;
            _response_headers.append(oss.str());
            std::string statusMsg = statusMessage(it->first);
            _response_headers.append(" " + statusMsg + "\r\n");
            _response_headers.append(" Content-Length: 0\r\n");
            _response_headers.append("Location: " + it->second + "\r\n");
            // Set the final response string
            _response_final = _response_headers;
        }
    }*/
    // Handle successful GET requests
    else if ((_request == "GET" || _request == "POST" || _request == "DELETE") && _code == 200)
    {
        // Set the final response string for successful GET
        setHeaders();
        _response_final = _response_headers + _response_body;
    }
    else
    {
        // For other methods or status codes, build error responses
        setHeaders();
        _response_final = _response_headers + _response_body;
        std::cout << "Final Response:\n"
                  << _response_body.size() << std::endl;
    }
}

void Response::setHeaders()
{
    // start fresh
    _response_headers.clear();

    statusLine();

    // Add standard headers
    // server();
    // appDate();

    // Content-Type (ensure a default if appContentType didn't add one)
    appContentType();
    if (_response_headers.find("Content-Type:") == std::string::npos)
        _response_headers.append("Content-Type: text/html; charset=utf-8\r\n");

    appContentLen();

    connection();
    _response_headers.append("\r\n");
}

 /*void Response::setHeaders()
{
    statusLine();
    appContentType();
    appContentLen();
    connection();
    _response_headers.append("\r\n");
}*/

// Checks if the request has an error status code set.
// If so, sets the response code accordingly and returns it.
// Otherwise, returns 0 indicating no error.
int Response::reqErr()
{
    if (!request.getErrorStatusCode().empty())
    {
        _code = std::atoi(request.getErrorStatusCode().c_str());
        return _code;
    }
    return 0;
}

std::string Response::getResponse()
{
    return _response_final;
}

Response::~Response()
{
}
