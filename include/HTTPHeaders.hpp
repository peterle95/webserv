/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPHeaders.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 15:41:24 by assistant         #+#    #+#             */
/*   Updated: 2025/09/13 15:41:24 by assistant        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPHEADERS_HPP
#define HTTPHEADERS_HPP

#include <string>
#include <map>
#include <sstream>
#include <vector>

/**
 * @brief Class responsible for parsing and managing HTTP headers
 * 
 * This class handles:
 * - Parsing header lines from HTTP request
 * - Validation of header names and values according to HTTP specifications
 * - Special handling for important headers (Content-Length, Transfer-Encoding, etc.)
 * - Case-insensitive header name lookup
 * - Error detection and reporting
 */
class HTTPHeaders
{
private:
    std::map<std::string, std::string> _headers; // Header key-value pairs (keys are lowercase)
    std::map<std::string, std::string> _originalKeys; // Original case mapping
    size_t _contentLength;              // Parsed Content-Length value
    bool _hasContentLength;             // Whether Content-Length header is present
    bool _isChunked;                    // Whether Transfer-Encoding: chunked
    std::string _contentType;           // Content-Type header value
    std::string _host;                  // Host header value
    std::string _connection;            // Connection header value
    std::string _transferEncoding;      // Transfer-Encoding header value
    bool _isValid;                      // Whether headers are valid
    std::string _errorMessage;          // Error message if parsing fails

public:
    HTTPHeaders();
    ~HTTPHeaders();
    
    // Main parsing method
    bool parseHeaders(std::istringstream& iss);
    
    // Header manipulation
    bool addHeader(const std::string& name, const std::string& value);
    bool hasHeader(const std::string& name) const;
    std::string getHeader(const std::string& name) const;
    void removeHeader(const std::string& name);
    
    // Specialized getters for important headers
    size_t getContentLength() const { return _contentLength; }
    bool hasContentLength() const { return _hasContentLength; }
    bool isChunked() const { return _isChunked; }
    const std::string& getContentType() const { return _contentType; }
    const std::string& getHost() const { return _host; }
    const std::string& getConnection() const { return _connection; }
    const std::string& getTransferEncoding() const { return _transferEncoding; }
    
    // General getters
    const std::map<std::string, std::string>& getAllHeaders() const { return _headers; }
    bool isValid() const { return _isValid; }
    const std::string& getErrorMessage() const { return _errorMessage; }
    size_t getHeaderCount() const { return _headers.size(); }
    
    // Validation
    bool validateHeaders() const;
    
    // Reset
    void reset();

private:
    // Helper methods
    bool parseHeaderLine(const std::string& line);
    void processSpecialHeaders(const std::string& lowerName, const std::string& value);
    void setError(const std::string& message);
    std::string toLowercase(const std::string& str) const;
    void trimTrailingCR(std::string& line);
    bool isEmptyLine(const std::string& line) const;
    
    // Content-Length parsing
    bool parseContentLength(const std::string& value);
    
    // Transfer-Encoding parsing
    bool parseTransferEncoding(const std::string& value);
};

#endif