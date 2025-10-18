#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

class Logger {
public:
    // Log raw HTTP request data with timestamp
    static void logRequest(const std::string &raw);

    // Log raw HTTP response data with timestamp
    static void logResponse(const std::string &raw);

private:
    // Build a timestamp string in the format: [YYYY-MM-DD HH:MM:SS]
    static std::string timestamp();
};

#endif