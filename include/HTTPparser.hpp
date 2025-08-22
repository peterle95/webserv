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
        std::string _HTTPrequest; // raw requested data, this might need to change type because large requests like POST might be too large for a string
        std::vector<std::string> _lines;
        std::string _method; // HTTP Method GET, POST, DELETE
        std::string _path; // Path of the requested resource
        std::string _version; // HTTP Version
        std::map<std::string, std::string> _headers; // Headers
        std::string _body; // Body
        std::string _buffer; // buffer to account for the sequential and potentially incremental nature of HTTP requests for non-blocking servers
        size_t _contentLength; // content length of the request
        std::string _errorStatusCode;         
    public:
        HTTPparser();
        ~HTTPparser();
};

#endif
