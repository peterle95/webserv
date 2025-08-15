/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorHandler.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/15 13:19:10 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/15 13:38:04 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Common.hpp"
#include <iostream>

void ErrorHandler::logError(const std::string &message) {
    std::cerr << "Error: " << message << std::endl;
}

void ErrorHandler::fatalError(const std::string &message) {
    logError(message);
    exit(EXIT_FAILURE);
}  

const char* ErrorHandler::ErrorMessage::what() const throw() {
    return "An error occurred in the web server.";
}