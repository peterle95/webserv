/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPBody.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 14:52:00 by assistant         #+#    #+#             */
/*   Updated: 2025/09/16 14:52:00 by assistant        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPBODY_HPP
#define HTTPBODY_HPP

#include <string>
#include <sstream>

// Forward declaration to avoid heavy includes in the header
class HTTPHeaders;

/*
  HTTPBody is responsible for parsing and holding the HTTP request body.
  It supports both fixed-length bodies (via Content-Length) and
  Transfer-Encoding: chunked, including optional trailers.

  - Input stream must be positioned at the first byte of the body (i.e.,
    immediately after the CRLF that ends the header section).
  - The HTTPHeaders instance must represent the headers for the same request.
  - On success, getBody() returns the raw body (no decoding beyond chunk
    framing). The component does not interpret Content-Type; it only frames.
*/
class HTTPBody
{
private:
    std::string _body;           // Parsed body content
    bool        _isValid;        // Whether the body was parsed successfully
    std::string _errorMessage;   // Error message when parsing fails

    // Helpers
    void setError(const std::string& message);
    void trimTrailingCR(std::string& line);

    bool parseFixedLengthBody(std::istringstream& iss, size_t length);
    bool parseChunkedBody(std::istringstream& iss);
    bool parseChunkSizeLine(std::istringstream& iss, size_t& outSize);
    bool readExact(std::istringstream& iss, size_t length, std::string& out);
    bool readCRLF(std::istringstream& iss);

public:
    HTTPBody();
    ~HTTPBody();

    // Parse based on headers; method is for debug
    bool parse(std::istringstream& iss, const HTTPHeaders& headers, const std::string& method);

    // Getters
    const std::string& getBody() const { return _body; }
    bool isValid() const { return _isValid; }
    const std::string& getErrorMessage() const { return _errorMessage; }

    // Reset state
    void reset();
};

#endif
