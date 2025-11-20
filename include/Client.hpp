#pragma once

#include "Common.hpp" // Include all necessary headers
#include "HTTPparser.hpp"
#include "Cgi.hpp"

// Forward declare to avoid circular dependencies
class Response;

// Defines the state of the client connection lifecycle
enum ClientState
{
    READING,
    GENERATING_RESPONSE,
    WRITING,
    CGI_WRITING_INPUT,
    CGI_READING_OUTPUT,
    CLOSING
};

class Client
{
public:
    // Constructor & Destructor
    Client(int fd, HttpServer &_server, size_t serverIndex, int serverPort);
    ~Client();

    // Main handler method called by the server
    void handleConnection();
    void checkCgiTimeout(); // NEW

    // Getters for the server to manage select()
    int getSocket() const;
    ClientState getState() const;

    // Getters for server context
    size_t getServerIndex() const { return _serverIndex; }
    int getServerPort() const { return _serverPort; }

    // CGI FD getters for select()
    int getCgiInputFd() const { return _cgi_pipe_in[1]; }
    int getCgiOutputFd() const { return _cgi_pipe_out[0]; }

private:
    // Private methods for internal logic
    void readRequest();
    void generateResponse();
    void writeResponse();
    size_t checkContentLength(const std::string &request, size_t header_end);

    // non-blocking CGI helpers
    void writeToCgi();
    void readFromCgi();
    void cleanup_cgi();
    // Helpers for checking request completeness
    bool hasChunked(const std::string &request, size_t header_end) const;
    bool checkEnd(const std::string &request, size_t header_end) const;

    // Member Variables
    int _socket;            // Thes client's socket file descriptor
    HttpServer &_server;    // Reference to the main server for config access
    Response *_response;    // Response object to build responses
    ClientState _state;     // The current state of the connection
    bool _keep_alive;       // Whether to keep the connection alive after response
    bool _peer_half_closed; // Peer performed shutdown(SHUT_WR); close after response

    // Buffers
    std::string _request_buffer;  // Stores raw request data as it's read
    std::string _response_buffer; // Stores the final, formatted response to be sent
    size_t _response_offset;      // Bytes already sent from _response_buffer

    // Parsers and Handlers
    HTTPparser _parser; // Parses the raw request
    CGI _cgi_handler;   // Manages the CGI process (project-specific wrapper)

    // CGI-specific state
    pid_t _cgi_pid;       // Process ID of the forked CGI script
    int _cgi_pipe_in[2];  // Pipe to send data TO the CGI script (parent writes to [1], child reads from [0])
    int _cgi_pipe_out[2]; // Pipe to receive data FROM the CGI script (child writes to [1], parent reads from [0])
    bool _cgi_started;    // Flag to indicate if the CGI process has been forked

    size_t _cgi_input_offset;       // Bytes of request body sent to CGI so far
    std::string _cgi_output_buffer; // Buffer to store output read from CGI
    time_t _cgi_start_time;         // NEW

    size_t _serverIndex; // Which server block this client belongs to
    int _serverPort;     // Which port client connected to
    int _status_code;    // HTTP status code for the response

    // Private copy constructor and assignment operator to prevent copying
    Client(const Client &other);
    // Client& operator=(const Client& other);
};
