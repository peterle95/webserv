/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Getters.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/18 11:46:14 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/18 11:46:14 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ErrorHandler.hpp"

int ErrorHandler::Exception::code() const 
{ 
    return _code; 
}

int ErrorHandler::Exception::line() const 
{ 
    return _line; 
}
const std::string& ErrorHandler::Exception::file() const 
{ 
    return _file; 
}