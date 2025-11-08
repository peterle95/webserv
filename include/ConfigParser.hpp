/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 13:20:54 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/22 17:06:03 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <set>
#include "ServerConfig.hpp"

class ConfigParser
{
private:
    std::string _configFile;
    std::string _root;
    std::string _index;

    size_t _clientMaxBodySize;
    std::vector<ServerConfig> _servers; // For multiple server blocks

    // add more directives
    std::vector<std::string> _lines;

    // Parse from a set of lines (expects raw lines; will trim/comment-strip internally)
    void parseLines(const std::vector<std::string> &lines);

    // Per-directive parsers
    void parseRoot(const std::string &val, size_t lineNo, std::string *root);
    void parseIndex(const std::string &val, size_t lineNo, std::string *index);
    void parseClientMaxBodySize(const std::string &val, size_t lineNo);

    // Helpers to keep parseLines small
    std::string preprocessLine(const std::string &raw);
    bool isBlockMarker(const std::string &line) const; // '{', '}', or contains '{'
    void requireSemicolon(const std::string &line, size_t lineNo) const;
    std::string stripTrailingSemicolon(const std::string &line) const;
    bool splitKeyVal(const std::string &line, std::string &key, std::string &val) const;
    void handleDirective(const std::string &key, const std::string &val, size_t lineNo);

public:
    //ConfigParser();
    // Construct directly from lines (e.g., read elsewhere)
    ConfigParser(ConfigParser const &other);
    ConfigParser(const std::vector<std::string> &lines);
    const std::vector<std::string> &getLines() const;
    ~ConfigParser();

    // Parse minimal NGINX-like config supporting: listen, root, index
    // Returns true on success, false on failure.
    bool parse(const std::string &path);

    // Accessors
    const std::string &getRoot() const;
    const std::string &getIndex() const;

    size_t getClientMaxBodySize() const;

    const std::vector<ServerConfig> &getServers() const;

    // TODO: implement error handling
    // TODO: implement parsing more directives (directives are the lines in the config file)
};

// Prototypes
std::string ltrim(const std::string &s);
std::string rtrim(const std::string &s);
std::string trim(const std::string &s);
std::string strip_comment(const std::string &s);

#endif