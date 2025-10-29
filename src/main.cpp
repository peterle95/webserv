/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/12 12:44:17 by pmolzer           #+#    #+#             */
/*   Updated: 2025/09/12 15:34:35 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"
#include "HttpServer.hpp"
#include "ConfigParser.hpp"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "IOUtils.hpp" // Removed: IOUtils wrappers include (tests removed)

int main(int argc, char **argv)
{
    std::string configPath = "conf/default.conf";

    // Removed: --run-tests support; main only starts the server
    if (argc == 2)
        configPath = argv[1];
    else if (argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " [conf/config_file.conf]" << std::endl;
        return 1;
    }

    ConfigParser parser;
    if (!parser.parse(configPath))
        return 1;
    HttpServer server(parser);
    return server.start();
}
