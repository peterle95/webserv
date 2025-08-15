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
            private:
                std::string message;
            public:
                ErrorMessage(const std::string &msg) : message(msg) {}
                virtual const char* what() const throw() {
                    return message.c_str();
                }
                virtual ~ErrorMessage() throw() { }     
        };
};

#endif