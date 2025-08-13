/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 13:20:54 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/13 13:20:54 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>

class ConfigParser
{
    private:
        std::string _configFile;
        int         _listenPort;
        std::string _root;
        std::string _index;
    public:
        ConfigParser();
        ~ConfigParser();

        // Parse minimal NGINX-like config supporting: listen, root, index
        // Returns true on success, false on failure.
        bool        parse(const std::string &path);

        // Accessors
        int                 getListenPort() const;
        const std::string&  getRoot() const;
        const std::string&  getIndex() const;
};

#endif