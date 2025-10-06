#include "Common.hpp"

void ConfigParser::applyAutoindex(LocationConfig *loc, const std::string &val, size_t lineNumber)
{
	if (val == "on")
		loc->autoindex = true;
	else if (val == "off")
		loc->autoindex = false;
	else
	{
		std::string msg = ErrorHandler::makeLocationMsg(
			std::string("Invalid value for autoindex (expected 'on' or 'off'): ") + val,
			(int)lineNumber, this->_configFile);
		throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE,
									  (int)lineNumber, this->_configFile);
	}
	DEBUG_PRINT("Set location autoindex -> " << (loc->autoindex ? "on" : "off"));
}

void ConfigParser::applyCgiPass(LocationConfig *loc, const std::string &val, size_t lineNumber)
{
	if (val == "on")
		loc->cgiPass = true;
	else if (val == "off")
		loc->cgiPass = false;
	else
	{
		std::string msg = ErrorHandler::makeLocationMsg(
			std::string("Invalid value for cgi_pass (expected 'on' or 'off'): ") + val,
			(int)lineNumber, this->_configFile);
		throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE,
									  (int)lineNumber, this->_configFile);
	}
	DEBUG_PRINT("Set location cgi_pass -> " << (loc->cgiPass ? "on" : "off"));
}

void ConfigParser::applyCgiExtension(LocationConfig *loc, const std::string &val, size_t lineNumber)
{
	if (val.empty() || val[0] != '.')
	{
		std::string msg = ErrorHandler::makeLocationMsg(
			std::string("Invalid cgi_extension (must start with '.'): ") + val,
			(int)lineNumber, this->_configFile);
		throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE,
									  (int)lineNumber, this->_configFile);
	}
	loc->cgiExtension = val;
	DEBUG_PRINT("Set location cgi_extension -> '" << loc->cgiExtension << "'");
}

// Handle location-specific directives
void ConfigParser::handleLocationDirective(LocationConfig *currentLocation,
										   const std::string &key,
										   const std::string &val,
										   size_t lineNumber)
{
	if (key == "root")
		parseRoot(val, lineNumber, &currentLocation->root);
	else if (key == "index")
		parseIndex(val, lineNumber, &currentLocation->index);
	else if (key == "allowed_methods")
		parseAllowedMethods(val, lineNumber, &currentLocation->allowedMethods);
	else if (key == "autoindex")
		applyAutoindex(currentLocation, val, lineNumber);
	else if (key == "cgi_pass")
		applyCgiPass(currentLocation, val, lineNumber);
	else if (key == "cgi_extension")
		applyCgiExtension(currentLocation, val, lineNumber);
	else
	{
		std::string msg = ErrorHandler::makeLocationMsg(
			std::string("Unknown directive in location block: ") + key,
			(int)lineNumber, this->_configFile);
		throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_UNKNOWN_DIRECTIVE,
									  (int)lineNumber, this->_configFile);
	}
}
