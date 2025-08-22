/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPparser.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/22 17:32:20 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/22 17:32:20 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPPARSER_HPP
#define HTTPPARSER_HPP

class HTTPparser
{
    private:
        std::string _HTTPrequest;
        std::vector<std::string> _lines;
        std::string _method; // HTTP Method GET, POST, DELETE
        std::string _path; // Path of the requested resource
        std::string _version; // HTTP Version
        std::map<std::string, std::string> _headers; // Headers
        std::string _body; // Body
    public:
        HTTPparser();
        ~HTTPparser();
};

#endif
