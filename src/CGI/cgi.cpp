#include "Cgi.hpp"
#include "Common.hpp"

// Utility function to convert number to string
std::string CGI::numberToString(int number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

// Default constructor
CGI::CGI()
	: cgi_pid_(-1)
{
	// Initialize pipes to invalid values
	pipe_in_[0] = -1;
	pipe_in_[1] = -1;
	pipe_out_[0] = -1;
	pipe_out_[1] = -1;

	// Initialize other members to empty/default values
	script_path_ = "";
	interpreter_path_ = "";
	request_body_ = "";
}

// Constructor
CGI::CGI(const HTTPparser &request)
	: cgi_pid_(-1)
{
	// Initialize pipes to invalid values
	pipe_in_[0] = -1;
	pipe_in_[1] = -1;
	pipe_out_[0] = -1;
	pipe_out_[1] = -1;

	// script_path_
	script_path_ = request.getCurrentFilePath();

	interpreter_path_ = "/usr/bin/env";
	request_body_ = request.getBody();

	setupEnvironment(request);
}

// Destructor
CGI::~CGI()
{
	cleanup();
}

// Setup environment variables
void CGI::setupEnvironment(const HTTPparser &request)
{
	const std::map<std::string, std::string> &headers = request.getHeaders();

	// Required CGI environment variables

	// Get the current system PATH and use it for the CGI environment.
	// This ensures /usr/bin/env can find interpreters (like python3.11) in the user's PATH.
	const char *sys_path = getenv("PATH");
	env_["PATH"] = sys_path ? sys_path : "/usr/bin:/bin:/usr/sbin:/sbin";
	env_["AUTH_TYPE"] = "";
	env_["CONTENT_TYPE"] = headers.count("content-type") > 0 ? headers.find("content-type")->second : "";
	env_["GATEWAY_INTERFACE"] = "CGI/1.1";
	env_["PATH_INFO"] = request.getPath();
	env_["PATH_TRANSLATED"] = script_path_;
	env_["REQUEST_METHOD"] = request.getMethod();
	env_["REQUEST_URI"] = request.getPath();
	env_["SCRIPT_NAME"] = request.getPath();
	env_["SCRIPT_FILENAME"] = script_path_;
	env_["SERVER_NAME"] = request.getServerName().empty() ? "localhost" : request.getServerName();
	env_["SERVER_PORT"] = request.getServerPort().empty() ? "8080" : request.getServerPort();
	env_["SERVER_PROTOCOL"] = "HTTP/1.1";
	env_["SERVER_SOFTWARE"] = "webserv/1.0";
	env_["CONTENT_LENGTH"] = numberToString(request_body_.size());

	// Add all HTTP headers as environment variables
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		 it != headers.end(); ++it)
	{
		// Skip transfer-encoding as it's server-internal framing
		if (it->first == "transfer-encoding")
			continue;
		std::string env_name = "HTTP_" + it->first;
		std::replace(env_name.begin(), env_name.end(), '-', '_');
		std::transform(env_name.begin(), env_name.end(), env_name.begin(), ::toupper);
		env_[env_name] = it->second;
	}
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
	char **args = new char *[4];
	if (!args)
		return NULL;

	args[0] = new char[interpreter_path_.size() + 1];
	strcpy(args[0], interpreter_path_.c_str());

	args[1] = new char[11];
	strcpy(args[1], "python3");

	args[2] = new char[script_path_.size() + 1];
	strcpy(args[2], script_path_.c_str());

	args[3] = NULL;
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

		// Write entire request body to CGI stdin (handle partial writes)
		if (!request_body_.empty())
		{
			const char *data = request_body_.data();
			size_t to_write = request_body_.size();
			while (to_write > 0)
			{
				ssize_t n = write(pipe_in_[1], data, to_write);
				if (n > 0)
				{
					data += n;
					to_write -= static_cast<size_t>(n);
				}
				else if (n == -1) // errno after write not allowed
				{
					continue; // Retry if interrupted by signal
				}
				else
				{
					std::cerr << "Warning: Failed to write request body to CGI" << std::endl;
					break;
				}
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
		DEBUG_PRINT("CGI executed successfully.");
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
