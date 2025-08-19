/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 13:20:54 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/19 17:37:01 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

class ConfigParser
{
    private:
        std::string _configFile;
        int         _listenPort;
        std::string _root;
        std::vector<std::string> _index; // to change implementation, vector because there might be multiple index files
        std::string _serverName; // to implement later
        std::map<int, std::string> _errorPage; // to implement later
        size_t _clientMaxBodySize; // to implement later
        std::string _host; // to implement later
        // add more directives
        std::set<std::string> _allowedMethods; // to implement later, set because order does not matter
        std::vector<std::string> _lines;

        // Parse from a set of lines (expects raw lines; will trim/comment-strip internally)
        void        parseLines(const std::vector<std::string>& lines);

        // Per-directive parsers
        void        parseListen(const std::string &val, size_t lineNo);
        void        parseRoot(const std::string &val, size_t lineNo);
        void        parseIndex(const std::string &val, size_t lineNo);
        void        parseServerName(const std::string &val, size_t lineNo);
        void        parseClientMaxBodySize(const std::string &val, size_t lineNo);
        void        parseAllowedMethods(const std::string &val, size_t lineNo);
        void        parseErrorPage(const std::string &val, size_t lineNo);

        // Helpers to keep parseLines small
        std::string preprocessLine(const std::string &raw);
        bool        isBlockMarker(const std::string &line) const; // '{', '}', or contains '{'
        void        requireSemicolon(const std::string &line, size_t lineNo) const;
        std::string stripTrailingSemicolon(const std::string &line) const;
        bool        splitKeyVal(const std::string &line, std::string &key, std::string &val) const;
        void        handleDirective(const std::string &key, const std::string &val, size_t lineNo);
    public:
        ConfigParser();
        // Construct directly from lines (e.g., read elsewhere)
        ConfigParser(const std::vector<std::string>& lines);
        ~ConfigParser();

        // Parse minimal NGINX-like config supporting: listen, root, index
        // Returns true on success, false on failure.
        bool        parse(const std::string &path);

        // Accessors
        int                 getListenPort() const;
        const std::string&  getRoot() const;
        const std::string&  getIndex() const;
        const std::vector<std::string>& getLines() const;

        // TODO: implement error handling
        // TODO: implement parsing more directives (directives are the lines in the config file)
};

// Prototypes
std::string ltrim(const std::string &s);
std::string rtrim(const std::string &s);
std::string trim(const std::string &s);
std::string strip_comment(const std::string &s);

#endif