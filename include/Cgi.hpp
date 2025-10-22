#ifndef CGI_HPP
#define CGI_HPP

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <iostream>
#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <cstdlib>
#include <fcntl.h>
#include <sstream>
#include <algorithm>
#include <cerrno>

// #include "HttpRequest.hpp"
// #include "Location.hpp" // location configuration class
#include "HttpServer.hpp"

#define CGI_BUFFER_SIZE 4096
#define CGI_TIMEOUT 30

class CGI
{
private:
	std::map<std::string, std::string> env_;
	std::string request_body_;
	std::string script_path_;
	std::string interpreter_path_;
	int pipe_in_[2];
	int pipe_out_[2];
	pid_t cgi_pid_;

	// Private helper functions
	void setupEnvironment(const HTTPparser &request, const HttpServer &server);
	char **createEnvArray() const;
	char **createArgsArray() const;
	void setupPipes();
	void closePipes();
	void handleChunkedBody();
	static std::string numberToString(int number);

public:
	CGI(const HTTPparser &request, const HttpServer &server);
	CGI(); // default constructor added for response.ccp
	~CGI();

	int execute();
	std::string readResponse();
	void cleanup();

	// Utility functions
	static std::string decodeChunkedBody(const std::string &body);
};

#endif
