/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Trim.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/16 17:32:38 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/16 17:32:38 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Common.hpp"

// helper functions, trim whitespace from left
std::string ltrim(const std::string &s)
{
    std::string::size_type i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r')) 
        i++;
    return s.substr(i);
}

// helper functions, trim whitespace from right
std::string rtrim(const std::string &s)
{
    if (s.empty())
        return s;
    std::string::size_type i = s.size();
    while (i > 0 && (s[i-1] == ' ' || s[i-1] == '\t' || s[i-1] == '\r'))
        i--;
    return s.substr(0, i);
}

// helper functions, trim whitespace from both sides
std::string trim(const std::string &s)
{ 
    return rtrim(ltrim(s)); 
}

// helper functions, strip comments
std::string strip_comment(const std::string &s)
{
    std::string::size_type pos = s.find('#');
    if (pos == std::string::npos)
        return s;
    return s.substr(0, pos);
}