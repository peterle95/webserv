
#ifndef PARSER_UTILS_HPP
#define PARSER_UTILS_HPP

#include <string>

class ParserUtils
{
public:
	// Stateless helpers shared by ConfigParser and ServerConfig
	static std::string preprocessLine(const std::string &raw);
	static bool isBlockMarker(const std::string &line);
	static std::string stripTrailingSemicolon(const std::string &line);
	static bool splitKeyVal(const std::string &line, std::string &key, std::string &val);
};

#endif
