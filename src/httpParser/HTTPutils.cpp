
#include "Common.hpp"

// Reset the parser to initial state
// Clears all internal state to prepare for a fresh parse. This method
// is called by the constructor and at the start of parseRequest.
void HTTPparser::reset()
{
    _state = PARSING_REQUEST_LINE;
    _HTTPrequest.clear();
    _requestLine.reset();
    _headers.reset();
    _body.clear();
    _buffer.clear();
    _errorStatusCode.clear();
    _isValid = false;
    _errorMessage.clear();
}

/*Set error state with message and status code
 */
void HTTPparser::setError(const std::string& message, const std::string& statusCode)
{
    // Mark the parser as failed and record a human-readable message and
    // an HTTP status code hint that higher layers can use for replies.
    _isValid = false;
    _errorMessage = message;
    _errorStatusCode = statusCode;
    _state = ERROR;
    DEBUG_PRINT("HTTPparser error: " << message << " (status: " << statusCode << ")");
}

// Set the internal parser state
// Only sets the enum; transitions are managed in parseRequest.
void HTTPparser::setState(State s)
{
    _state = s;
}

// Remove trailing carriage return if present
void HTTPparser::trimTrailingCR(std::string& line)
{
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
}
