/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/12 12:44:17 by pmolzer           #+#    #+#             */
/*   Updated: 2025/09/12 15:34:35 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"
#include "HttpServer.hpp"
#include "ConfigParser.hpp"

void testhttpParsing()
{
    std::cout << "=== Testing Improved HTTP Parser ===" << std::endl;
    
    // Example HTTP request
    std::string testRequest = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0 (Test Browser)\r\n"
        "Accept: text/html,application/xhtml+xml\r\n"
        "Accept-Language: en-US,en;q=0.9\r\n"
        "Connection: keep-alive\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "key1=value1&key2=value2";
    
    // Create parser and test parsing
    HTTPparser parser;
    
    std::cout << "\n--- Testing Valid Request ---" << std::endl;
    if (parser.parseRequest(testRequest))
    {
        std::cout << "✓ Request parsed successfully!" << std::endl;
        std::cout << "Method: " << parser.getMethod() << std::endl;
        std::cout << "Path: " << parser.getPath() << std::endl;
        std::cout << "Version: " << parser.getVersion() << std::endl;
        std::cout << "Headers: " << parser.getHeaders().size() << " total" << std::endl;
        std::cout << "Host: " << parser.getHeader("Host") << std::endl;
        std::cout << "Content-Length: " << parser.getContentLength() << std::endl;
        std::cout << "Body length: " << parser.getBody().length() << " characters" << std::endl;
    }
    else
    {
        std::cout << "✗ Failed to parse request" << std::endl;
        std::cout << "Error: " << parser.getErrorMessage() << std::endl;
        std::cout << "Status: " << parser.getErrorStatusCode() << std::endl;
    }
    
    // Test invalid request
    std::cout << "\n--- Testing Invalid Request ---" << std::endl;
    std::string invalidRequest = "INVALID REQUEST LINE\r\n";
    
    HTTPparser parser2;
    if (parser2.parseRequest(invalidRequest))
    {
        std::cout << "✗ Should have failed but didn't!" << std::endl;
    }
    else
    {
        std::cout << "✓ Correctly rejected invalid request" << std::endl;
        std::cout << "Error: " << parser2.getErrorMessage() << std::endl;
        std::cout << "Status: " << parser2.getErrorStatusCode() << std::endl;
    }
    
    // Test directory traversal protection
    std::cout << "\n--- Testing Security Features ---" << std::endl;
    std::string maliciousRequest = 
        "GET /../../../etc/passwd HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HTTPparser parser3;
    if (parser3.parseRequest(maliciousRequest))
    {
        std::cout << "✗ Security vulnerability: directory traversal not blocked!" << std::endl;
    }
    else
    {
        std::cout << "✓ Security check passed: directory traversal blocked" << std::endl;
        std::cout << "Error: " << parser3.getErrorMessage() << std::endl;
    }
    
    std::cout << "\n=== Test Complete ===" << std::endl;
} 

// implement later try...catch blocks
int main(int argc, char **argv)
{
    // Accept 0 or 1 argument. If none, use default config path.
    std::string configPath = "conf/default.conf";
    if (argc == 2)
        configPath = argv[1];
    else if (argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " conf/[config_file].conf" << std::endl;
        return 1;
    }
    ConfigParser parser;
    if (!parser.parse(configPath))
        return 1;
    
    testhttpParsing();
    HttpServer server(parser.getListenPort(), parser.getRoot(), parser.getIndex());
    return server.start();
}
