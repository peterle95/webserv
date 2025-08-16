/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 13:20:54 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/16 16:19:52 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP
#include <string>
#include <vector>

class ConfigParser
{
    private:
        std::string _configFile;
        int         _listenPort;
        std::string _root;
        std::string _index;
        std::string _serverName; // to implement later
        std::string _errorPage; // to implement later
        std::string _clientMaxBodySize; // to implement later
        std::string _host; // to implement later
        // add more directives
        std::vector<std::string> _lines;

        // Parse from a set of lines (expects raw lines; will trim/comment-strip internally)
        void        parseLines(const std::vector<std::string>& lines);
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

#endif