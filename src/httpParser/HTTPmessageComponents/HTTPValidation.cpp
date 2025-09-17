/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPValidation.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 15:41:24 by assistant         #+#    #+#             */
/*   Updated: 2025/09/13 15:41:24 by assistant        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPValidation.hpp"
#include <cctype>
#include <sstream>

// HTTP Method validation
bool HTTPValidation::isValidMethod(const std::string& method)
{
    std::set<std::string> validMethods = getValidMethods();
    return validMethods.find(method) != validMethods.end();
}

std::set<std::string> HTTPValidation::getValidMethods()
{
    std::set<std::string> methods;
    methods.insert("GET");
    methods.insert("POST");
    methods.insert("DELETE");
    return methods;
}

// HTTP Version validation
bool HTTPValidation::isValidVersion(const std::string& version)
{
    std::set<std::string> supportedVersions = getSupportedVersions();
    return supportedVersions.find(version) != supportedVersions.end();
}

std::set<std::string> HTTPValidation::getSupportedVersions()
{
    std::set<std::string> versions;
    versions.insert("HTTP/1.0");
    versions.insert("HTTP/1.1");
    return versions;
}

// Path validation with security checks
bool HTTPValidation::isValidPath(const std::string& path)
{
    // Check if path is empty or doesn't start with '/'
    if (path.empty() || path[0] != '/')
        return false;
    
    // Check for directory traversal attempts
    if (containsDirectoryTraversal(path))
        return false;
    
    // Check for invalid characters (basic validation)
    for (size_t i = 0; i < path.length(); ++i)
    {
        char c = path[i];
        // Allow only printable ASCII characters and some special chars commonly used in URLs
        if (c < 32 || c > 126)
            return false;
        
        // Disallow some potentially dangerous characters
        if (c == '<' || c == '>' || c == '"' || c == '|' || c == '^' || c == '`' || c == '{' || c == '}')
            return false;
    }
    
    return true;
}

bool HTTPValidation::containsDirectoryTraversal(const std::string& path)
{
    // Check for various directory traversal patterns
    return (path.find("../") != std::string::npos ||
            path.find("..\\") != std::string::npos ||
            path.find("/..") != std::string::npos ||
            path.find("\\..") != std::string::npos ||
            path == ".." ||
            path.find("/../") != std::string::npos ||
            path.find("\\..\\") != std::string::npos);
}

std::string HTTPValidation::sanitizePath(const std::string& path)
{
    // Simple sanitization - remove directory traversal patterns
    std::string sanitized = path;
    
    // Replace backslashes with forward slashes
    for (size_t i = 0; i < sanitized.length(); ++i)
    {
        if (sanitized[i] == '\\')
            sanitized[i] = '/';
    }
    
    // Remove consecutive slashes
    std::string result;
    bool lastWasSlash = false;
    for (size_t i = 0; i < sanitized.length(); ++i)
    {
        if (sanitized[i] == '/')
        {
            if (!lastWasSlash)
                result += sanitized[i];
            lastWasSlash = true;
        }
        else
        {
            result += sanitized[i];
            lastWasSlash = false;
        }
    }
    
    return result;
}

// Header validation
bool HTTPValidation::isValidHeaderName(const std::string& name)
{
    if (name.empty())
        return false;
    
    return isToken(name);
}

bool HTTPValidation::isValidHeaderValue(const std::string& value)
{
    return !containsInvalidChars(value);
}

bool HTTPValidation::containsInvalidChars(const std::string& value)
{
    for (size_t i = 0; i < value.length(); ++i)
    {
        char c = value[i];
        // Check for CR, LF, or NUL characters as per RFC
        if (c == '\r' || c == '\n' || c == '\0')
            return true;
        
        // Check for other control characters (except TAB which is allowed)
        if (isControlChar(c) && c != '\t')
            return true;
    }
    return false;
}

// String utility functions
std::string HTTPValidation::trim(const std::string& str)
{
    return trimLeft(trimRight(str));
}

std::string HTTPValidation::trimLeft(const std::string& str)
{
    size_t start = 0;
    while (start < str.length() && std::isspace(static_cast<unsigned char>(str[start])))
        ++start;
    return str.substr(start);
}

std::string HTTPValidation::trimRight(const std::string& str)
{
    size_t end = str.length();
    while (end > 0 && std::isspace(static_cast<unsigned char>(str[end - 1])))
        --end;
    return str.substr(0, end);
}

// Content-Length validation
bool HTTPValidation::isValidContentLength(const std::string& value, size_t& length)
{
    if (value.empty())
        return false;
    
    // RFC 7230 allows only decimal digits for Content-Length. Reject
    // any non-digit character to avoid ambiguity.
    // Check if all characters are digits
    for (size_t i = 0; i < value.length(); ++i)
    {
        if (!std::isdigit(value[i]))
            return false;
    }
    
    // Convert to size_t
    std::istringstream iss(value);
    iss >> length;
    
    return !iss.fail();
}

// Helper methods
bool HTTPValidation::isToken(const std::string& str)
{
    if (str.empty())
        return false;
    
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (!isValidChar(str[i]))
            return false;
    }
    return true;
}

bool HTTPValidation::isValidChar(char c)
{
    // Token characters as per HTTP specification
    // CHAR = any US-ASCII character (octets 0 - 127)
    // token = 1*<any CHAR except CTLs or separators>
    // separators = "(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\" | <">
    //              | "/" | "[" | "]" | "?" | "=" | "{" | "}" | SP | HT
    
    if (c <= 31 || c >= 127) // Control characters and non-ASCII
        return false;
    
    // Check for separator characters
    const std::string separators = "()<>@,;:\\\"/[]?={} \t";
    return separators.find(c) == std::string::npos;
}

bool HTTPValidation::isControlChar(char c)
{
    return (c >= 0 && c <= 31) || c == 127;
}