#include "Common.hpp" // Include all necessary headers
#include "HTTPparser.hpp"
#include "HttpResponse.hpp"
#include "Cgi.hpp"

// Forward declare to avoid circular dependencies
class HttpServer;

// Defines the state of the client connection lifecycle
enum ClientState {
    READING,
    GENERATING_RESPONSE,
    WRITING,
    AWAITING_CGI,
    CLOSING
};

class Client {
public:
    // Constructor & Destructor
    Client(int socket, HttpServer& server);
    ~Client();

    // Main handler method called by the server
    void handleConnection();

    // Getters for the server to manage select()
    int getSocket() const;
    ClientState getState() const;

private:
    // Private methods for internal logic
    void readRequest();
    void generateResponse();
    void writeResponse();
    void handleCgi();
    void startCgi();

    // Member Variables
    int _socket;                      // The client's socket file descriptor
    HttpServer& _server;              // Reference to the main server for config access
    ClientState _state;               // The current state of the connection
    bool        _keep_alive;          // Whether to keep the connection alive after response

    // Buffers
    std::string _request_buffer;      // Stores raw request data as it's read
    std::string _response_buffer;     // Stores the final, formatted response to be sent
    size_t      _response_offset;     // Bytes already sent from _response_buffer

    // Parsers and Handlers
    HTTPparser   _parser;             // Parses the raw request
    Response    *_response;           // Builds the HTTP response (project-specific)
    CGI          _cgi_handler;        // Manages the CGI process (project-specific wrapper)

    // CGI-specific state
    pid_t _cgi_pid;                   // Process ID of the forked CGI script
    int   _cgi_pipe_in[2];            // Pipe to send data TO the CGI script (parent writes to [1], child reads from [0])
    int   _cgi_pipe_out[2];           // Pipe to receive data FROM the CGI script (child writes to [1], parent reads from [0])
    bool  _cgi_started;               // Flag to indicate if the CGI process has been forked

    // Private copy constructor and assignment operator to prevent copying
    Client(const Client& other);
    Client& operator=(const Client& other);
};
