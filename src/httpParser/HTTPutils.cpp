
#include "Common.hpp"

/**
 * @brief Reset the parser to initial state
 */
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

/**
 * @brief Set error state with message and status code
 * 
 * @param message Detailed error message
 * @param statusCode HTTP status code (default: "400")
 */
void HTTPparser::setError(const std::string& message, const std::string& statusCode)
{
    _isValid = false;
    _errorMessage = message;
    _errorStatusCode = statusCode;
    _state = ERROR;
    DEBUG_PRINT("HTTPparser error: " << message << " (status: " << statusCode << ")");
}