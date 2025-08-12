/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/12 12:44:17 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/12 12:44:17 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Common.hpp"


int main(int argc, char **argv)
{
    /*inside if statement, change this to a try catch block, 
    and overload the << operator for the error messages*/
    if (argc == 1 || argc == 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return (1);
        // either user selects a config file or a default config file is used
    }
    else 
    {
        std::cerr << "Too many arguments" << std::endl;
        return (1);
    }
    return (0);
}
