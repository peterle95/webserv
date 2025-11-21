#include "Common.hpp"

static int createSocket()
{
    // Create a new socket for the server.
    // AF_INET     -> IPv4 addressing (use AF_INET6 for IPv6).
    // SOCK_STREAM -> TCP (reliable, connection-oriented byte stream).
    // protocol=0  -> Let the system choose the default protocol for SOCK_STREAM (which is TCP).
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        std::cerr << "socket() failed" << std::endl;
        return -1;
    }

    return server_fd;
}

static bool setSocketReusable(int server_fd)
{
    /*When a TCP connection is closed, the operating system doesn't immediately free up the port.
    Instead, it places the socket in a TIME_WAIT state for a short period (typically 30-120 seconds).
    This is a safety feature to ensure any delayed data packets from the old connection are
    properly discarded and don't interfere with a new connection.

    Without SO_REUSEADDR, if you stop your server (e.g., with Ctrl+C)
    and try to restart it right away, the bind() call will fail because the
    OS still considers the port "in use" by the lingering TIME_WAIT socket. */
    int opt = 1;
    // tells the operating system's kernel to modify its default behavior.
    // "Allow my program to bind() to this port, even if a connection on the
    // same port is stuck in the TIME_WAIT state."
    return setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
}

// Initializes the socket address structure with the specified host and port.
static void initializeAddress(struct sockaddr_in *addr, int port, in_addr_t host)
{
    // Zeros out the struct to ensure all fields are initialized, preventing unpredictable behavior.
    std::memset(addr, 0, sizeof(*addr));

    // Sets the address family to AF_INET, specifying that the socket will use the IPv4 protocol.
    addr->sin_family = AF_INET;

    // Sets the IP address. 
    addr->sin_addr.s_addr = host;

    // Sets the port number. htons() converts the port number from the host's
    // byte order to network byte order, which is crucial for interoperability across different systems.
    addr->sin_port = htons((uint16_t)port);
}

// Binds the socket to the address created from the host and port.
static bool bindSocket(int server_fd, int port, in_addr_t host)
{
    // Bind address
    struct sockaddr_in addr; // Declares a struct to hold address information for an IPv4 socket.

    initializeAddress(&addr, port, host);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        // Create a character buffer to hold the human-readable string version of the IP address.
        // INET_ADDRSTRLEN is a constant (usually 16) defining the max length of an IPv4 string (e.g. "255.255.255.255\0").
        char host_str[INET_ADDRSTRLEN];

        // Create a temporary struct specifically to hold the binary IP address.
        // This is required as an argument for the conversion function below.
        struct in_addr addr_struct;

        // Assign the input binary IP (host) to the struct's parameter.
        addr_struct.s_addr = host;

        // inet_ntop (Network TO Presentation): Converts the binary IP address into a readable string.
        // AF_INET specifies we are working with IPv4.
        // writes the result into 'host_str'.
        inet_ntop(AF_INET, &addr_struct, host_str, INET_ADDRSTRLEN);
        std::cerr << "bind() failed on port " << port << " host " << host_str << std::endl;
        return false;
    }

    return true;
}

static bool startListening(int server_fd)
{
    if (listen(server_fd, 128) < 0)
    {
        std::cerr << "listen() failed" << std::endl;
        return false;
    }

    return true;
}

bool HttpServer::setNonBlocking(int fd)
{
    // Query the current file status flags for this file descriptor (fd).
    // These are not filesystem permissions, but runtime flags that control how
    // the kernel handles I/O on this fd (e.g., append mode, blocking/non-blocking).
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return false; // Failed to retrieve flags

    // Add the O_NONBLOCK flag to the existing set of flags.
    // O_NONBLOCK means:
    //   - read() will return immediately if no data is available (instead of blocking)
    //   - write() will return immediately if it cannot accept more data
    // The bitwise OR ensures we *preserve* existing flags (e.g., O_APPEND).
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return false; // Failed to update flags

    // Success: the socket is now in non-blocking mode.
    return true;
}

// Configures the socket for reuse and binds it to the specified host and port.
static bool configureSocket(int server_fd, int port, in_addr_t host)
{
    if (!setSocketReusable(server_fd))
    {
        std::cerr << "Failed to set SO_REUSEADDR" << std::endl;
        return false;
    }

    if (!bindSocket(server_fd, port, host))
    {
        return false;
    }

    if (!startListening(server_fd))
        return false;

    if (!HttpServer::setNonBlocking(server_fd))
    {
        std::cerr << "Failed to set server socket non-blocking" << std::endl;
        return false;
    }

    return true;
}

// Creates and binds a socket to the specified host and port.
int HttpServer::createAndBindSocket(int port, in_addr_t host)
{
    int server_fd = createSocket();
    if (server_fd < 0)
    {
        return -1;
    }

    if (!configureSocket(server_fd, port, host))
    {
        close(server_fd);
        return -1;
    }

    return server_fd;
}