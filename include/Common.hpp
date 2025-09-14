/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Common.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 13:21:03 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/13 15:15:35 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMMON_HPP
#define COMMON_HPP

/*A header file that includes common standard library headers and defines shared constants or 
macros used throughout the project.*/

#include <iostream>
#include <sys/socket.h> // socket(), bind(), listen(), accept()
#include <unistd.h> // close()
#include <string> 
#include <fstream> // file stream: readFileToString()
#include <sstream> // string stream: sendAll()
#include <cctype> // character classification: isdigit()
#include <cstdlib> // standard library: exit()
#include <netinet/in.h> // internet address: struct sockaddr_in
#include <arpa/inet.h> // internet address: inet_addr()
#include <fcntl.h> // file control: O_RDONLY
#include <cstring> // string operations: memset()
#include <csignal> // signal handling: SIGINT, SIGTERM
#include <cerrno> // error handling: errno
#include <map>
#include <set>
#include <vector>
#include "ConfigParser.hpp" // configuration parser 
#include "HttpServer.hpp" // HTTP server
#include "ErrorHandler.hpp"
#include "HTTPparser.hpp" // parser for HTTP requests sent by the client
#include "HTTPValidation.hpp" // HTTP validation utilities
#include "HTTPRequestLine.hpp" // HTTP request line parser
#include "HTTPHeaders.hpp" // HTTP headers parser

// create a DEBUG macro so that if it's true the debugging mode in the code will print stuff
#ifndef DEBUG
#define DEBUG true
#endif
#if DEBUG
#define DEBUG_PRINT(x) std::cout << x << std::endl
#else
#define DEBUG_PRINT(x)
#endif

#endif
