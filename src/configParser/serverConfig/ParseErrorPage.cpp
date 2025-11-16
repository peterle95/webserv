/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseErrorPage.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 15:05:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/20 17:26:59 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"

void ServerConfig::parseErrorPage(const std::string &val, size_t lineNo)
{
  // Syntax: error_page <code> [code ...] <uri>
  std::istringstream iss(val);
  std::vector<std::string> parts;
  std::string tok;
  while (iss >> tok)
    parts.push_back(tok);
  if (parts.size() < 2)
  {
    std::string msg = ErrorHandler::makeLocationMsg("error_page requires at least a code and a target", (int)lineNo, this->_configFile);
    throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
  }
  std::string target = parts.back();
  for (size_t i = 0; i < parts.size() - 1; ++i)
  {
    int code;
    try
    {
      code = std::atoi(parts[i].c_str());
      if (code < 400 || code > 599)
      {
        std::string msg = ErrorHandler::makeLocationMsg("Status code in error page must be between 400 and 599", (int)lineNo, this->_configFile);
        throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
      }
      _errorPage[code] = target;
    }
    catch (const std::exception &e)
    {
      std::string msg = ErrorHandler::makeLocationMsg("Invalid status code in error_page_directive", (int)lineNo, this->_configFile);
      throw ErrorHandler::Exception(msg, ErrorHandler::CONFIG_INVALID_DIRECTIVE, (int)lineNo, this->_configFile);
    }
  }
}
