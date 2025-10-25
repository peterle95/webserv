
#include "Common.hpp" // for strip_comment, trim, DEBUG_PRINT
#include "ParserUtils.hpp"

std::string ParserUtils::preprocessLine(const std::string &raw)
{
	std::string line = strip_comment(raw);
	line = trim(line);
	return line;
}

bool ParserUtils::isBlockMarker(const std::string &line)
{
	return (line == "}" || line.find('{') != std::string::npos);
}

std::string ParserUtils::stripTrailingSemicolon(const std::string &line)
{
	if (!line.empty() && line[line.size() - 1] == ';')
		return line.substr(0, line.size() - 1);
	return line;
}

bool ParserUtils::splitKeyVal(const std::string &line, std::string &key, std::string &val)
{
	std::string::size_type sp = line.find(' ');
	if (sp == std::string::npos)
		return false;
	key = trim(line.substr(0, sp));
	val = trim(line.substr(sp + 1));
	return true;
}