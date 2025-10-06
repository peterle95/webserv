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

static void initializeAddress(struct sockaddr_in* addr, int port)
{
    // Zeros out the struct to ensure all fields are initialized, preventing unpredictable behavior.
    std::memset(addr, 0, sizeof(*addr));
    
    // Sets the address family to AF_INET, specifying that the socket will use the IPv4 protocol.
    addr->sin_family = AF_INET;
    
    // Sets the IP address. INADDR_ANY is a special address (0.0.0.0) that 
    // tells the socket to listen on all available network interfaces of the machine. 
    // htonl() converts this value from the host's byte order to the standard network byte order.
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    
    // Sets the port number. htons() converts the port number from the host's 
    // byte order to network byte order, which is crucial for interoperability across different systems.
    addr->sin_port = htons((uint16_t)port);
}

static bool bindSocket(int server_fd, int port)
{
    // Bind address
    struct sockaddr_in addr; // Declares a struct to hold address information for an IPv4 socket.
    
    initializeAddress(&addr, port);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "bind() failed on port " << port << std::endl;
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

static bool configureSocket(int server_fd, int port)
{
    if (!setSocketReusable(server_fd))
    {
        std::cerr << "Failed to set SO_REUSEADDR" << std::endl;
        return false;
    }
    
    if (!bindSocket(server_fd, port))
        return false;
    
    if (!startListening(server_fd))
        return false;
    
    if (!HttpServer::setNonBlocking(server_fd))
    {
        std::cerr << "Failed to set server socket non-blocking" << std::endl;
        return false;
    }
    
    return true;
}

int HttpServer::createAndBindSocket()
{
    int server_fd = createSocket();
    if (server_fd < 0)
    {
        return -1;
    }
    
    if (!configureSocket(server_fd, _port))
    {
        close(server_fd);
        return -1;
    }
    
    return server_fd;
}