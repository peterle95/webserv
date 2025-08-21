/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/12 12:44:17 by pmolzer           #+#    #+#             */
<<<<<<< HEAD
/*   Updated: 2025/08/15 13:35:59 by pmolzer          ###   ########.fr       */
=======
/*   Updated: 2025/08/16 15:51:40 by pmolzer          ###   ########.fr       */
>>>>>>> b488101cc1622b45f220c77dc2a260835d9ab971
/*                                                                            */
/* ************************************************************************** */

#include "../include/Common.hpp"
#include "../include/HttpServer.hpp"

// implement later try...catch blocks
int main(int argc, char **argv)
{
    // Accept 0 or 1 argument. If none, use default config path.
    std::string configPath = "conf/default.conf";
    if (argc == 2)
        configPath = argv[1];
    else if (argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " conf/[config_file].conf" << std::endl;
        return 1;
    }
    ConfigParser parser;
    try
    {
        parser.parse(configPath);
    }
    catch(const ErrorHandler::ErrorMessage& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
<<<<<<< HEAD
    }
=======

    if(DEBUG){
>>>>>>> b488101cc1622b45f220c77dc2a260835d9ab971
    // For now, just show parsed values to verify the minimal parser works
    std::cout << "Config loaded from: " << configPath << std::endl;
    std::cout << "listen: " << parser.getListenPort() << std::endl;
    std::cout << "root:   " << parser.getRoot() << std::endl;
    std::cout << "index:  " << parser.getIndex() << std::endl;

    // Demonstrate access to all raw lines read from the config file
    const std::vector<std::string>& lines = parser.getLines();
    for (size_t i = 0; i < lines.size(); ++i)
        std::cout << "CFG[" << i << "]: " << lines[i] << std::endl;
    }
    std::string example = "print test";
    DEBUG_PRINT("DEBUG_PRINT: " << example << std::endl);
    HttpServer server(parser.getListenPort(), parser.getRoot(), parser.getIndex());
    return server.start();
}
