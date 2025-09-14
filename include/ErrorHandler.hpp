/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorHandler.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/18 11:08:47 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/19 17:32:47 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

/* Minimal error handling skeleton used across the project.
 * For now, only parser/config related errors are defined and thrown.
 */

#include <exception>
#include <string>

class ErrorHandler
{
  public:
    // Minimal set of error categories we currently care about
    enum ErrorCode {
        CONFIG_FILE_NOT_FOUND = 1,
        CONFIG_INVALID_PORT,
        CONFIG_MISSING_SEMICOLON,
        CONFIG_INVALID_DIRECTIVE,
        CONFIG_UNKNOWN_DIRECTIVE
    };

    // Lightweight exception carrying a message, code, and optional location
    class Exception : public std::exception {
      private:
        std::string _msg;
        int         _code;
        int         _line;      // -1 if unknown
        std::string _file;      // empty if unknown

      public:
        Exception(const std::string &message, int code);
        Exception(const std::string &message, int code, int line, const std::string &file);
        virtual ~Exception() throw();

        const char* what() const throw();
        int         code() const;             // ErrorCode value
        int         line() const;             // line number
        const std::string& file() const;      // file path
    };

    // Helper to build a formatted message with location context
    static std::string makeLocationMsg(const std::string &prefix, int line, const std::string &file);

    // Map an ErrorCode to a short label (no allocations beyond std::string construction)
    static const char* codeToString(int code);

    /* TODO:
     * - Add HTTP status mapping and default error page selection (SUBJECT: default error pages).
     * - Add validation helpers for all config directives (body size, methods, redirections, etc.).
     * - Centralize logging policy (respect non-blocking constraints, no errno-based flow after I/O).
     * - Extend codes for runtime server errors (socket/bind/listen, CGI, filesystem ops).
     * - Provide user-friendly messages and per-route context once routing is implemented.
     */
};

#endif
