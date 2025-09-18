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
    std::cout << RED << "=== Testing Improved HTTP Parser ===" << RESET << std::endl;
    
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
    
    std::cout << RED << "\n--- Testing Valid Request ---" << RESET << std::endl;
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
    std::cout << RED << "\n--- Testing Invalid Request ---" << RESET << std::endl;
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
    std::cout << RED << "\n--- Testing Security Features ---" << RESET << std::endl;
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

    // =============================================================
    // Additional tests for HTTP body parsing
    // =============================================================
    std::cout << RED << "\n=== Additional HTTP Body Parsing Tests ===" << RESET << std::endl;

    // 1) Fixed-length body (exact match)
    std::cout << RED << "\n--- Fixed-Length Body: Exact Match ---" << RESET << std::endl;
    std::string fixedExact =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "hello=world!!"; // 13 bytes
    HTTPparser p_fixedExact;
    if (p_fixedExact.parseRequest(fixedExact))
    {
        std::cout << "✓ Parsed fixed-length body" << std::endl;
        std::cout << "Body length: " << p_fixedExact.getBody().length() << std::endl;
        if (p_fixedExact.getBody().length() == 13)
            std::cout << "✓ Body length matches Content-Length" << std::endl;
        else
            std::cout << "✗ Body length does not match Content-Length" << std::endl;
    }
    else
    {
        std::cout << "✗ Failed to parse fixed-length exact body" << std::endl;
        std::cout << "Error: " << p_fixedExact.getErrorMessage() << std::endl;
    }

    // 2) Fixed-length body (shorter than Content-Length)
    std::cout << RED << "\n--- Fixed-Length Body: Too Short ---" << RESET << std::endl;
    std::string fixedShort =
        "POST /short HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 10\r\n"
        "\r\n"
        "short"; // only 5 bytes
    HTTPparser p_fixedShort;
    if (!p_fixedShort.parseRequest(fixedShort))
    {
        std::cout << "✓ Correctly rejected short body" << std::endl;
        std::cout << "Error: " << p_fixedShort.getErrorMessage() << std::endl;
        std::cout << "Status: " << p_fixedShort.getErrorStatusCode() << std::endl;
    }
    else
    {
        std::cout << "✗ Should have failed due to short body" << std::endl;
    }

    // 3) Fixed-length body (longer than Content-Length): parser reads only declared bytes
    std::cout << RED << "\n--- Fixed-Length Body: Longer Than Declared ---" << RESET << std::endl;
    std::string fixedLong =
        "POST /long HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "1234567"; // 7 bytes provided, expect body == "12345"
    HTTPparser p_fixedLong;
    if (p_fixedLong.parseRequest(fixedLong))
    {
        std::cout << "✓ Parsed fixed-length body (reads only declared bytes)" << std::endl;
        std::cout << "Body: '" << p_fixedLong.getBody() << "' (len=" << p_fixedLong.getBody().length() << ")" << std::endl;
        if (p_fixedLong.getBody() == "12345")
            std::cout << "✓ Body content matches declared length" << std::endl;
        else
            std::cout << "✗ Body content mismatch" << std::endl;
    }
    else
    {
        std::cout << "✗ Failed to parse fixed-length longer body" << std::endl;
    }

    // 4) Chunked encoding (standard example)
    std::cout << RED << "\n--- Chunked Body: Standard ---" << RESET << std::endl;
    std::string chunked =
        "POST /chunked HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "4\r\n"
        "Wiki\r\n"
        "5\r\n"
        "pedia\r\n"
        "0\r\n"
        "\r\n";
    HTTPparser p_chunked;
    if (p_chunked.parseRequest(chunked))
    {
        std::cout << "✓ Parsed chunked body" << std::endl;
        std::cout << "Body: '" << p_chunked.getBody() << "'" << std::endl;
        if (p_chunked.getBody() == "Wikipedia")
            std::cout << "✓ Body content correct (Wikipedia)" << std::endl;
        else
            std::cout << "✗ Body content incorrect" << std::endl;
    }
    else
    {
        std::cout << "✗ Failed to parse chunked body" << std::endl;
        std::cout << "Error: " << p_chunked.getErrorMessage() << std::endl;
    }

    // 5) Chunked encoding with trailers
    std::cout << RED << "\n--- Chunked Body: With Trailers ---" << RESET << std::endl;
    std::string chunkedTrailers =
        "POST /chunked HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "3\r\n"
        "abc\r\n"
        "2\r\n"
        "de\r\n"
        "0\r\n"
        "Header-One: x\r\n"
        "Header-Two: y\r\n"
        "\r\n";
    HTTPparser p_chunkedTrailers;
    if (p_chunkedTrailers.parseRequest(chunkedTrailers))
    {
        std::cout << "✓ Parsed chunked body with trailers" << std::endl;
        if (p_chunkedTrailers.getBody() == "abcde")
            std::cout << "✓ Body content correct (abcde)" << std::endl;
        else
            std::cout << "✗ Body content incorrect" << std::endl;
    }
    else
    {
        std::cout << "✗ Failed to parse chunked body with trailers" << std::endl;
        std::cout << "Error: " << p_chunkedTrailers.getErrorMessage() << std::endl;
    }

    // 6) Chunked encoding with invalid size (non-hex)
    std::cout << RED << "\n--- Chunked Body: Invalid Chunk Size ---" << RESET << std::endl;
    std::string chunkedInvalid =
        "POST /chunked HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "Z\r\n"  // invalid hex
        "oops\r\n"
        "0\r\n"
        "\r\n";
    HTTPparser p_chunkedInvalid;
    if (!p_chunkedInvalid.parseRequest(chunkedInvalid))
    {
        std::cout << "✓ Correctly rejected invalid chunk size" << std::endl;
        std::cout << "Error: " << p_chunkedInvalid.getErrorMessage() << std::endl;
    }
    else
    {
        std::cout << "✗ Should have failed for invalid chunk size" << std::endl;
    }

    // 7) No body with GET (no Content-Length/Transfer-Encoding)
    std::cout << RED << "\n--- No Body with GET ---" << RESET << std::endl;
    std::string getNoBody =
        "GET /plain HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    HTTPparser p_getNoBody;
    if (p_getNoBody.parseRequest(getNoBody))
    {
        std::cout << "✓ Parsed GET without body" << std::endl;
        if (p_getNoBody.getBody().empty())
            std::cout << "✓ Body is empty as expected" << std::endl;
        else
            std::cout << "✗ Body should be empty" << std::endl;
    }
    else
    {
        std::cout << "✗ Failed to parse GET without body" << std::endl;
    }
    
    std::cout << RED << "\n=== Test Complete ===" << RESET << std::endl;
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
    if(DEBUG)
    {
        // test http parsing
        testhttpParsing();
    }
    HttpServer server(parser.getListenPort(), parser.getRoot(), parser.getIndex());
    HttpServer server(parser.getListenPort(), parser.getServerName());
    return server.start();
}
