/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseClientMaxBodySize.cpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 15:05:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/20 17:22:25 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../../include/Common.hpp"

void ConfigParser::parseClientMaxBodySize(const std::string &val, size_t lineNo)
{
    // Accept formats like: 1024, 10k, 1m, 1g (case-insensitive); store in bytes
    // Ensure a value exists (error if missing)

    // Split the input into a number part and a multiplier (based on suffix like k, m, or g)

    // Parse the numeric value, validate

    // Check for overflow by confirming reverse calculation is consistent

    // Store the final byte value in the config struct

    // Optionally print debug info
}
