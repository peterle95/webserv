/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/12 12:44:17 by pmolzer           #+#    #+#             */
/*   Updated: 2025/09/05 13:13:17 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Common.hpp"
#include "../include/HttpServer.hpp"

// implement later try...catch blocks
int main(int argc, char **argv)
{
    // Accept 0 or 1 argument. If none, use default config path.
    std::string configPath = "Config/default.conf";
    if (argc == 2)
        configPath = argv[1];
    else if (argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " conf/[config_file].conf" << std::endl;
        return 1;
    }

    ConfigParser parser;
    if (!parser.parseConfigFile(configPath))
        return 1;

    if(DEBUG){
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
    /* std::string example = "print test";
    DEBUG_PRINT("DEBUG_PRINT: " << example << std::endl); */
    HttpServer server(parser.getListenPort(), parser.getRoot(), parser.getIndex());
    return server.start();
}
