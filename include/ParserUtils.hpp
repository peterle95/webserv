
#ifndef PARSER_UTILS_HPP
#define PARSER_UTILS_HPP

#include <string>

namespace ParserUtils
{

	// Stateless helpers shared by ConfigParser and ServerConfig
	std::string preprocessLine(const std::string &raw);
	bool isBlockMarker(const std::string &line);
	std::string stripTrailingSemicolon(const std::string &line);
	bool splitKeyVal(const std::string &line, std::string &key, std::string &val);

} // namespace ParserUtils

#endif
