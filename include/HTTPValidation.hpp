/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPValidation.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 15:41:24 by assistant         #+#    #+#             */
/*   Updated: 2025/09/13 15:41:24 by assistant        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPVALIDATION_HPP
#define HTTPVALIDATION_HPP

#include <string>
#include <set>

/**
 * @brief Utility class for HTTP validation functions
 * 
 * This class provides static methods to validate various HTTP components
 * such as methods, versions, paths, and header values according to RFC specifications.
 */
class HTTPValidation
{
public:
    // HTTP Method validation
    static bool isValidMethod(const std::string& method);
    static std::set<std::string> getValidMethods();
    
    // HTTP Version validation
    static bool isValidVersion(const std::string& version);
    static std::set<std::string> getSupportedVersions();
    
    // Path validation (security checks for directory traversal, etc.)
    static bool isValidPath(const std::string& path);
    static bool containsDirectoryTraversal(const std::string& path);
    static std::string sanitizePath(const std::string& path);
    
    // Header validation
    static bool isValidHeaderName(const std::string& name);
    static bool isValidHeaderValue(const std::string& value);
    static bool containsInvalidChars(const std::string& value);
    
    // String utilities
    static std::string trim(const std::string& str);
    static std::string trimLeft(const std::string& str);
    static std::string trimRight(const std::string& str);
    
    // Content-Length validation
    static bool isValidContentLength(const std::string& value, size_t& length);
    
private:
    HTTPValidation(); // Prevent instantiation
    
    // Helper methods
    static bool isToken(const std::string& str);
    static bool isValidChar(char c);
    static bool isControlChar(char c);
};

#endif