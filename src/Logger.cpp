#include "Logger.hpp"
#include "Common.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

static std::string twoDigits(int v) {
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << v;
    return oss.str();
}

std::string Logger::timestamp() {
    std::time_t t = std::time(NULL);
    std::tm *lt = std::localtime(&t);

    std::string ts = "[";
    ts += twoDigits(1900 + lt->tm_year);
    ts += "-";
    ts += twoDigits(1 + lt->tm_mon);
    ts += "-";
    ts += twoDigits(lt->tm_mday);
    ts += " ";
    ts += twoDigits(lt->tm_hour);
    ts += ":";
    ts += twoDigits(lt->tm_min);
    ts += ":";
    ts += twoDigits(lt->tm_sec);
    ts += "]";
    return ts;
}

static void printLine(const std::string &prefix, const std::string &raw) {
#ifdef DEBUG
    std::cout << prefix << " " << raw << std::endl;
#else
    (void)prefix; (void)raw;
#endif
}

void Logger::logRequest(const std::string &raw) {
    printLine(timestamp() + " [REQUEST]", raw);
}

void Logger::logResponse(const std::string &raw) {
    printLine(timestamp() + " [RESPONSE]", raw);
}