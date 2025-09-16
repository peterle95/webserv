/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPBody.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 14:52:00 by assistant         #+#    #+#             */
/*   Updated: 2025/09/16 14:52:00 by assistant        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPBody.hpp"
#include "HTTPHeaders.hpp"
#include "HTTPValidation.hpp"
#include "Common.hpp"
#include <vector>
#include <cctype>

HTTPBody::HTTPBody()
    : _isValid(false)
{
    reset();
}

HTTPBody::~HTTPBody()
{
}

void HTTPBody::reset()
{
    _body.clear();
    _isValid = false;
    _errorMessage.clear();
}

void HTTPBody::setError(const std::string& message)
{
    _isValid = false;
    _errorMessage = message;
    DEBUG_PRINT("HTTPBody error: " << message);
}

void HTTPBody::trimTrailingCR(std::string& line)
{
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
}

bool HTTPBody::readExact(std::istringstream& iss, size_t length, std::string& out)
{
    // Attempt to read exactly 'length' bytes from the stream. If fewer
    // bytes are available (e.g., truncated request), return false.
    if (length == 0)
    {
        out.clear();
        return true;
    }
    std::vector<char> buf(length);
    iss.read(&buf[0], static_cast<std::streamsize>(length));
    std::streamsize got = iss.gcount();
    if (static_cast<size_t>(got) != length)
        return false;
    out.assign(&buf[0], &buf[0] + length);
    return true;
}

bool HTTPBody::readCRLF(std::istringstream& iss)
{
    // Consume CRLF exactly. Some clients may send LF only, but HTTP/1.1
    // requires CRLF; we enforce CRLF here for stricter compliance.
    char cr = 0, lf = 0;
    if (!iss.get(cr)) return false;
    if (!iss.get(lf)) return false;
    return (cr == '\r' && lf == '\n');
}

bool HTTPBody::parseChunkSizeLine(std::istringstream& iss, size_t& outSize)
{
    // Read a single "chunk-size[;chunk-extension]" line and parse the size
    // (hexadecimal). Extensions are ignored for now.
    std::string line;
    if (!std::getline(iss, line))
    {
        setError("Unexpected end of input while reading chunk size");
        return false;
    }
    trimTrailingCR(line);

    // Remove chunk extensions if present
    std::string::size_type scPos = line.find(';');
    if (scPos != std::string::npos)
        line = line.substr(0, scPos);

    // Trim whitespace
    line = HTTPValidation::trim(line);
    if (line.empty())
    {
        setError("Empty chunk size line");
        return false;
    }

    // Validate hex digits and parse
    for (size_t i = 0; i < line.size(); ++i)
    {
        if (!std::isxdigit(static_cast<unsigned char>(line[i])))
        {
            setError("Invalid chunk size: non-hex character found");
            return false;
        }
    }

    unsigned long parsed = 0;
    std::istringstream hexss(line);
    hexss >> std::hex >> parsed;
    if (hexss.fail())
    {
        setError("Failed to parse chunk size");
        return false;
    }
    outSize = static_cast<size_t>(parsed);
    DEBUG_PRINT("Chunk size: " << outSize);
    return true;
}

bool HTTPBody::parseChunkedBody(std::istringstream& iss)
{
    // Implements RFC 7230 chunked coding:
    //  chunk = chunk-size [; chunk-ext] CRLF
    //          chunk-data CRLF
    //  last-chunk = 1*('0') [; chunk-ext] CRLF
    //               trailer-section CRLF
    _body.clear();
    while (true)
    {
        size_t chunkSize = 0;
        if (!parseChunkSizeLine(iss, chunkSize))
            return false;

        if (chunkSize == 0)
        {
            // End of chunks: consume CRLF that terminates the zero-length chunk
            if (!readCRLF(iss))
            {
                setError("Missing CRLF after zero-size chunk");
                return false;
            }
            // Read optional trailer headers until an empty line
            std::string trailer;
            while (std::getline(iss, trailer))
            {
                trimTrailingCR(trailer);
                if (trailer.empty())
                    break; // End of trailers
                // Optionally, handle trailer headers here
            }
            DEBUG_PRINT("Finished parsing chunked body, total size: " << _body.size());
            return true;
        }

        // Read exactly 'chunkSize' bytes of data for this chunk.
        std::string chunk;
        if (!readExact(iss, chunkSize, chunk))
        {
            setError("Chunk data shorter than declared size");
            return false;
        }
        _body.append(chunk);

        // Each chunk is followed by CRLF
        if (!readCRLF(iss))
        {
            setError("Missing CRLF after chunk data");
            return false;
        }
    }
}

bool HTTPBody::parseFixedLengthBody(std::istringstream& iss, size_t length)
{
    // Content-Length driven parsing: read exactly N bytes and store.
    _body.clear();
    std::string data;
    if (!readExact(iss, length, data))
    {
        setError("Body shorter than Content-Length");
        return false;
    }
    _body = data;
    return true;
}

bool HTTPBody::parse(std::istringstream& iss, const HTTPHeaders& headers, const std::string& method)
{
    // Decide body parsing strategy using the already-parsed headers.
    // Method is informational for future enhancements (e.g., validating
    // that certain methods typically do not include a body).
    (void)method; // Currently unused; kept for future logic and debug
    _isValid = false;
    _errorMessage.clear();

    if (headers.isChunked())
    {
        DEBUG_PRINT("HTTPBody: parsing Transfer-Encoding: chunked");
        if (!parseChunkedBody(iss))
            return false;
        _isValid = true;
        return true;
    }
    if (headers.hasContentLength())
    {
        size_t len = headers.getContentLength();
        DEBUG_PRINT("HTTPBody: parsing Content-Length: " << len);
        if (!parseFixedLengthBody(iss, len))
            return false;
        _isValid = true;
        return true;
    }

    // No length provided: read remaining bytes as-is
    DEBUG_PRINT("HTTPBody: no explicit length; reading remaining stream");
    std::ostringstream bodyStream;
    bodyStream << iss.rdbuf();
    _body = bodyStream.str();
    _isValid = true;
    return true;
}
