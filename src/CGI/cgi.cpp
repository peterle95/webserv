#include "../../include/Cgi.hpp"

// Utility function to convert number to string
std::string CGI::numberToString(int number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

// Constructor
CGI::CGI(const HttpRequest &request, const Location &location, const Server &server)
	: cgi_pid_(-1)
{
	// Initialize pipes to invalid values
	pipe_in_[0] = -1;
	pipe_in_[1] = -1;
	pipe_out_[0] = -1;
	pipe_out_[1] = -1;

	script_path_ = getTargetFileFullPath(request, location);
	setupEnvironment(request, location, server);
	determineInterpreter();
	request_body_ = request.getBody();

	// Handle chunked encoding if needed
	const std::map<std::string, std::string> &headers = request.getHeaders();
	if (headers.find("Transfer-Encoding") != headers.end() &&
		headers.find("Transfer-Encoding")->second == "chunked")
	{
		handleChunkedBody();
	}
}

// Destructor
CGI::~CGI()
{
	cleanup();
}

// Get full target file path
std::string CGI::getTargetFileFullPath(const HttpRequest &request, const Location &location) const
{
	std::string root = location.getRoot();
	std::string path = request.getPath();

	// Ensure root doesn't end with slash and path starts with slash
	if (!root.empty() && root[root.size() - 1] == '/')
	{
		root = root.substr(0, root.size() - 1);
	}
	if (!path.empty() && path[0] != '/')
	{
		path = "/" + path;
	}

	return root + path;
}

// Setup environment variables
void CGI::setupEnvironment(const HttpRequest &request, const Location &location, const Server &server)
{
	const std::map<std::string, std::string> &headers = request.getHeaders();

	// Required CGI environment variables
	env_["AUTH_TYPE"] = "";
	env_["CONTENT_TYPE"] = headers.count("Content-Type") > 0 ? headers.find("Content-Type")->second : "";
	env_["GATEWAY_INTERFACE"] = "CGI/1.1";
	env_["PATH_INFO"] = request.getPath();
	env_["PATH_TRANSLATED"] = script_path_;
	env_["QUERY_STRING"] = request.getQuery();
	env_["REMOTE_ADDR"] = "127.0.0.1"; // Should be actual client IP
	env_["REMOTE_HOST"] = "localhost";
	env_["REQUEST_METHOD"] = request.getMethod();
	env_["REQUEST_URI"] = request.getPath();
	env_["SCRIPT_NAME"] = request.getPath();
	env_["SCRIPT_FILENAME"] = script_path_;
	env_["SERVER_NAME"] = headers.count("Host") > 0 ? headers.find("Host")->second : "localhost";
	env_["SERVER_PORT"] = numberToString(server.getPort());
	env_["SERVER_PROTOCOL"] = "HTTP/1.1";
	env_["SERVER_SOFTWARE"] = "webserv/1.0";
	env_["CONTENT_LENGTH"] = numberToString(request_body_.size());

	// Add all HTTP headers as environment variables
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		 it != headers.end(); ++it)
	{
		std::string env_name = "HTTP_" + it->first;
		std::replace(env_name.begin(), env_name.end(), '-', '_');
		std::transform(env_name.begin(), env_name.end(), env_name.begin(), ::toupper);
		env_[env_name] = it->second;
	}
}

// Determine interpreter based on file extension
void CGI::determineInterpreter()
{
	size_t dot_pos = script_path_.find_last_of(".");
	if (dot_pos != std::string::npos)
	{
		std::string extension = script_path_.substr(dot_pos);

		if (extension == ".php")
		{
			interpreter_path_ = "/usr/bin/php";
		}
		else if (extension == ".py")
		{
			interpreter_path_ = "/usr/bin/python";
		}
		else if (extension == ".pl")
		{
			interpreter_path_ = "/usr/bin/perl";
		}
		else if (extension == ".sh")
		{
			interpreter_path_ = "/bin/sh";
		}
		else
		{
			interpreter_path_ = "";
		}
	}
}

// Handle chunked request body
void CGI::handleChunkedBody()
{
	request_body_ = decodeChunkedBody(request_body_);
	env_["CONTENT_LENGTH"] = numberToString(request_body_.size());
}

// Decode chunked body
std::string CGI::decodeChunkedBody(const std::string &body)
{
	std::string unchunked;
	size_t pos = 0;
	size_t body_size = body.size();

	while (pos < body_size)
	{
		// Find chunk size line
		size_t line_end = body.find("\r\n", pos);
		if (line_end == std::string::npos)
			break;

		std::string size_line = body.substr(pos, line_end - pos);
		char *endptr;
		long chunk_size = strtol(size_line.c_str(), &endptr, 16);

		if (chunk_size == 0)
			break; // Last chunk
		if (endptr == size_line.c_str())
			break; // Invalid chunk size

		// Move to chunk data
		pos = line_end + 2;
		if (pos + chunk_size > body_size)
			break;

		unchunked.append(body.substr(pos, chunk_size));
		pos += chunk_size + 2; // Skip CRLF (\r\n) after chunk
	}

	return unchunked;
}

// Create environment array for execve
char **CGI::createEnvArray() const
{
	char **envp = new char *[env_.size() + 1];
	if (!envp)
		return NULL;

	int i = 0;
	for (std::map<std::string, std::string>::const_iterator it = env_.begin();
		 it != env_.end(); ++it)
	{
		std::string env_var = it->first + "=" + it->second;
		envp[i] = new char[env_var.size() + 1];
		strcpy(envp[i], env_var.c_str());
		i++;
	}
	envp[i] = NULL;

	return envp;
}

// Create arguments array for execve
char **CGI::createArgsArray() const
{
	char **args = new char *[3];
	if (!args)
		return NULL;

	args[0] = new char[interpreter_path_.size() + 1];
	strcpy(args[0], interpreter_path_.c_str());

	args[1] = new char[script_path_.size() + 1];
	strcpy(args[1], script_path_.c_str());

	args[2] = NULL;
	return args;
}

// Setup pipes
void CGI::setupPipes()
{
	if (pipe(pipe_in_) == -1)
	{
		std::cerr << "Error: Input pipe creation failed: " << strerror(errno) << std::endl;
	}
	if (pipe(pipe_out_) == -1)
	{
		std::cerr << "Error: Output pipe creation failed: " << strerror(errno) << std::endl;
		close(pipe_in_[0]);
		close(pipe_in_[1]);
	}
}

// Close pipes
void CGI::closePipes()
{
	if (pipe_in_[0] != -1)
		close(pipe_in_[0]);
	if (pipe_in_[1] != -1)
		close(pipe_in_[1]);
	if (pipe_out_[0] != -1)
		close(pipe_out_[0]);
	if (pipe_out_[1] != -1)
		close(pipe_out_[1]);

	pipe_in_[0] = pipe_in_[1] = pipe_out_[0] = pipe_out_[1] = -1;
}

// Execute CGI
int CGI::execute()
{
	if (interpreter_path_.empty())
	{
		std::cerr << "Error: No interpreter found for script: " << script_path_ << std::endl;
		return -1;
	}

	// Check if script exists and is executable
	struct stat stat_buf;
	if (stat(script_path_.c_str(), &stat_buf) == -1)
	{
		std::cerr << "Error: Script not found: " << script_path_ << std::endl;
		return -1;
	}
	if (!S_ISREG(stat_buf.st_mode))
	{
		std::cerr << "Error: Not a regular file: " << script_path_ << std::endl;
		return -1;
	}
	if (access(script_path_.c_str(), X_OK) == -1)
	{
		std::cerr << "Error: Script not executable: " << script_path_ << std::endl;
		return -1;
	}

	setupPipes();
	if (pipe_in_[0] == -1 || pipe_out_[0] == -1)
	{
		return -1;
	}

	cgi_pid_ = fork();
	if (cgi_pid_ == -1)
	{
		std::cerr << "Error: Fork failed: " << strerror(errno) << std::endl;
		closePipes();
		return -1;
	}

	if (cgi_pid_ == 0)
	{ // Child process (CGI)
		// Set up I/O redirection
		dup2(pipe_in_[0], STDIN_FILENO);
		dup2(pipe_out_[1], STDOUT_FILENO);
		dup2(pipe_out_[1], STDERR_FILENO);

		closePipes();

		// Change to script directory for relative paths
		size_t last_slash = script_path_.find_last_of("/");
		if (last_slash != std::string::npos)
		{
			std::string dir = script_path_.substr(0, last_slash);
			if (chdir(dir.c_str()) == -1)
			{
				std::cerr << "Warning: Could not change to script directory" << std::endl;
			}
		}

		// Prepare environment and arguments
		char **envp = createEnvArray();
		char **args = createArgsArray();

		if (!envp || !args)
		{
			exit(EXIT_FAILURE);
		}

		execve(interpreter_path_.c_str(), args, envp);

		// If execve fails
		std::cerr << "Error: execve failed: " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}
	else
	{						 // Parent process (server)
		close(pipe_in_[0]);	 // Close read end of input pipe
		close(pipe_out_[1]); // Close write end of output pipe

		// Write request body to CGI stdin
		if (!request_body_.empty())
		{
			ssize_t written = write(pipe_in_[1], request_body_.c_str(), request_body_.size());
			if (written == -1)
			{
				std::cerr << "Warning: Failed to write request body to CGI" << std::endl;
			}
		}
		close(pipe_in_[1]); // Close write end after writing

		return 0;
	}
}

// Read CGI response
std::string CGI::readResponse()
{
	if (cgi_pid_ == -1 || pipe_out_[0] == -1)
	{
		return "HTTP/1.1 500 Internal Server Error\r\n\r\nCGI not executed properly";
	}

	std::string response;
	char buffer[CGI_BUFFER_SIZE];
	ssize_t bytes_read;

	// Read until EOF (handles cases without Content-Length)
	while ((bytes_read = read(pipe_out_[0], buffer, sizeof(buffer))) > 0)
	{
		response.append(buffer, bytes_read);
	}

	close(pipe_out_[0]);
	pipe_out_[0] = -1;

	// Wait for CGI process to finish
	int status;
	pid_t result = waitpid(cgi_pid_, &status, 0);
	cgi_pid_ = -1;

	if (result == -1)
	{
		std::cerr << "Error: waitpid failed: " << strerror(errno) << std::endl;
		return "HTTP/1.1 500 Internal Server Error\r\n\r\nCGI process error";
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
	{
		return response;
	}
	else
	{
		std::cerr << "Error: CGI execution failed with status: " << status << std::endl;
		return "HTTP/1.1 500 Internal Server Error\r\n\r\nCGI execution failed";
	}
}

// Cleanup resources
void CGI::cleanup()
{
	if (cgi_pid_ != -1)
	{
		kill(cgi_pid_, SIGKILL);
		waitpid(cgi_pid_, NULL, 0);
		cgi_pid_ = -1;
	}
	closePipes();
}
