/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequestLine.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 15:41:24 by assistant         #+#    #+#             */
/*   Updated: 2025/09/13 15:41:24 by assistant        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUESTLINE_HPP
#define HTTPREQUESTLINE_HPP

#include <string>
#include <sstream>

/**
 * @brief Class responsible for parsing and validating HTTP request lines
 * 
 * The request line is the first line of an HTTP request and contains:
 * Method SP Request-URI SP HTTP-Version CRLF
 * Example: GET /index.html HTTP/1.1
 * 
 * This class handles:
 * - Parsing the method, path, and version from the request line
 * - Validation of each component according to HTTP specifications  
 * - Error detection and reporting
 */
class HTTPRequestLine
{
private:
    std::string _method;    // HTTP Method (GET, POST, DELETE, etc.)
    std::string _path;      // Path of the requested resource
    std::string _version;   // HTTP Version (HTTP/1.0, HTTP/1.1)
    std::string _rawLine;   // Original request line for debugging
    bool _isValid;          // Whether the request line is valid
    std::string _errorMessage; // Error message if parsing fails

public:
    HTTPRequestLine();
    ~HTTPRequestLine();
    
    // Main parsing method
    bool parseRequestLine(const std::string& requestLine);
    
    // Validation methods
    bool validateMethod(const std::string& method);
    bool validatePath(const std::string& path);
    bool validateVersion(const std::string& version);
    
    // Getters
    const std::string& getMethod() const { return _method; }
    const std::string& getPath() const { return _path; }
    const std::string& getVersion() const { return _version; }
    const std::string& getRawLine() const { return _rawLine; }
    bool isValid() const { return _isValid; }
    const std::string& getErrorMessage() const { return _errorMessage; }
    
    // Setters (mainly for testing)
    void setMethod(const std::string& method) { _method = method; }
    void setPath(const std::string& path) { _path = path; }
    void setVersion(const std::string& version) { _version = version; }
    
    // Reset the object to initial state
    void reset();
    
private:
    // Helper methods
    void setError(const std::string& message);
    bool parseComponents(std::istringstream& iss);
    void trimTrailingCR(std::string& line);
};

#endif