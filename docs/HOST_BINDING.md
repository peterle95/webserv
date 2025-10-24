# Host/IP Address Binding Feature

## Overview

This feature allows webserv to bind to specific network interfaces by specifying host addresses in the configuration file. This is particularly useful for:

- **Security**: Restricting the server to listen only on localhost (127.0.0.1)
- **Multi-homed systems**: Binding to specific network interfaces on machines with multiple IP addresses
- **Development**: Running multiple server instances on the same machine with different IP addresses

## Configuration Syntax

### Basic Syntax

```nginx
server {
    listen <host>:<port>;
    # ... other directives
}
```

### Examples

#### 1. Listen on localhost only

```nginx
server {
    listen 127.0.0.1:8080;
    server_name localhost;
    # ...
}
```

This configuration makes the server accessible only from the local machine.

#### 2. Listen on all interfaces (default behavior)

```nginx
server {
    listen 0.0.0.0:8080;  # Explicit
    # OR
    listen 8080;          # Implicit - defaults to 0.0.0.0
    # ...
}
```

The server will accept connections from any network interface.

#### 3. Listen on specific IP address

```nginx
server {
    listen 192.168.1.100:8080;
    server_name internal.local;
    # ...
}
```

Useful for multi-homed systems where you want to bind to a specific network interface.

#### 4. Multiple listen directives

```nginx
server {
    listen 127.0.0.1:8080;      # localhost
    listen 192.168.1.100:8080;  # LAN interface
    listen 0.0.0.0:8081;        # all interfaces, different port
    # ...
}
```

The same server block can listen on multiple host:port combinations.

## Implementation Details

### Configuration Parsing

The `parseListen()` function in `ServerConfig` parses the listen directive:

- Extracts host and port from the format `host:port`
- Defaults to `0.0.0.0` if no host is specified
- Validates the port number
- Stores host:port pairs in `_listenAddresses` vector

### Socket Binding

The `createAndBindSocket()` function in `HttpServer` has been updated:

- Takes both `host` and `port` parameters
- Uses `inet_pton()` to convert IP string to binary format
- Supports:
  - `0.0.0.0` - binds to all interfaces (INADDR_ANY)
  - `127.0.0.1` - binds to localhost
  - Any valid IPv4 address - binds to specific interface

### Server Startup

During server startup:

1. Iterates through all server blocks
2. For each server block, iterates through all listen addresses
3. Creates and binds a socket for each host:port pair
4. Sets sockets to non-blocking mode
5. Stores socket information including host, port, and server index

## Testing

### Test localhost binding

```bash
# Start server with localhost binding
./webserv conf/host_binding.conf

# Should work - connecting from localhost
curl http://127.0.0.1:8080/

# Should fail - connecting from external IP (if different from 127.0.0.1)
curl http://<your-external-ip>:8080/
```

### Test specific IP binding

```bash
# Find your network interfaces
ifconfig  # or ip addr show

# Update configuration with your specific IP
# Edit conf/host_binding.conf:
# listen <your-ip>:8080;

# Start server
./webserv conf/host_binding.conf

# Test from same machine
curl http://<your-ip>:8080/

# Test from another machine on same network
curl http://<your-ip>:8080/
```

### Test multiple interfaces

```bash
# Configuration with multiple listen directives
server {
    listen 127.0.0.1:8080;
    listen 192.168.1.100:8080;
    # ...
}

# Both should work:
curl http://127.0.0.1:8080/
curl http://192.168.1.100:8080/
```

## Error Handling

### Invalid IP Address

If an invalid IP address is specified, the server:

1. Logs an error message: "Invalid IP address: <address>"
2. Falls back to INADDR_ANY (0.0.0.0)
3. Continues with other listen directives

### Bind Failure

If binding fails (e.g., address already in use, permission denied):

1. Logs an error: "bind() failed on <host>:<port>"
2. Continues with other listen directives
3. Server starts if at least one socket binds successfully

### Common Issues

1. **Permission denied**: Ports below 1024 require root privileges
   ```bash
   sudo ./webserv conf/host_binding.conf  # Use sudo for privileged ports
   ```

2. **Address already in use**: Another process is using the port
   ```bash
   # Find process using the port
   lsof -i :<port>
   # Or
   netstat -tulpn | grep :<port>
   ```

3. **Cannot assign requested address**: IP address doesn't exist on the system
   ```bash
   # Check available addresses
   ip addr show
   ```

## Backward Compatibility

The implementation maintains backward compatibility:

- Old config files without host specification still work
- `listen 8080;` is equivalent to `listen 0.0.0.0:8080;`
- Legacy `_ports` and `_hosts` vectors are still populated
- Existing code using `getListenPorts()` continues to work

## Future Enhancements

Potential improvements:

1. **IPv6 support**: Extend to support IPv6 addresses
2. **Unix domain sockets**: Support for `listen unix:/path/to/socket;`
3. **Hostname resolution**: Support `listen example.com:8080;` with DNS lookup
4. **Listen options**: Support additional options like `listen 8080 default_server;`

## Technical Notes

### Data Structures

```cpp
// Primary storage (C++98 compatible)
std::vector<std::pair<std::string, int> > _listenAddresses;

// Backward compatibility
std::vector<std::string> _hosts;
std::vector<int> _ports;

// Socket tracking
struct ServerSocketInfo {
    int socket_fd;
    std::string host;
    int port;
    size_t serverIndex;
};
```

### Key Functions

- `ServerConfig::parseListen()` - Parses listen directive
- `ServerConfig::getListenAddresses()` - Returns host:port pairs
- `HttpServer::createAndBindSocket()` - Creates and binds socket to specific host
- `initializeAddress()` - Converts host string to sockaddr_in structure

## References

- NGINX listen directive: https://nginx.org/en/docs/http/ngx_http_core_module.html#listen
- BSD socket API: `man 2 bind`, `man 3 inet_pton`
- IPv4 addressing: RFC 791
