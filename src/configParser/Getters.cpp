/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Getters.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/16 17:30:53 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/16 17:30:53 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"

int ConfigParser::getListenPort() const 
{ 
    return _listenPort; 
}

const std::string& ConfigParser::getRoot() const 
{ 
    return _root; 
}

const std::string &ConfigParser::getIndex() const
{
    return _index;
}

const std::vector<std::string>& ConfigParser::getLines() const
{
    return _lines;
}