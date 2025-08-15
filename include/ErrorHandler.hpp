#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

#include <string>

class ErrorHandler
{
    public:
        static void logError(const std::string &message);
        static void fatalError(const std::string &message);
        class ErrorMessage : public std::exception
        {
            public:
                virtual const char* what() const throw();
        };
};

#endif