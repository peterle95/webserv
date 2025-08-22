/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseServerName.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 15:05:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/19 15:05:00 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../../include/Common.hpp"

void ConfigParser::parseServerName(const std::string &val, size_t /*lineNo*/)
{
    this->_serverName = val;
    DEBUG_PRINT("Applied server_name -> '" << this->_serverName << "'");
}
