# Feature: Host/IP Address Binding

## Branch: feature/host-ip-binding

This branch implements the ability to bind the webserv HTTP server to specific network interfaces using host/IP addresses defined in the configuration file.

## What's New

### Configuration Support

The server now supports specifying both host and port in the `listen` directive:

```nginx
server {
    listen 127.0.0.1:8080;      # Listen on localhost only
    listen 192.168.1.100:8081;  # Listen on specific IP
    listen 0.0.0.0:8082;        # Listen on all interfaces
    listen 8083;                 # Shorthand: defaults to 0.0.0.0:8083
}
```

### Files Modified

1. **include/ServerConfig.hpp**
   - Uncommented `_listenAddresses` vector to store host:port pairs
   - Added `getListenAddresses()` accessor method

2. **src/configParser/serverConfig/ParseListen.cpp**
   - Updated to parse host from `listen` directive
   - Extracts IP address before the colon
   - Defaults to `0.0.0.0` if no host specified
   - Stores in `_listenAddresses` vector

3. **src/configParser/serverConfig/ServerConfig.cpp**
   - Added `_listenAddresses` to constructor initialization
   - Implemented `getListenAddresses()` accessor

4. **include/HttpServer.hpp**
   - Updated `ServerSocketInfo` to include `host` field
   - Changed `createAndBindSocket()` signature to accept host parameter

5. **src/server/BindSocket.cpp**
   - Updated `initializeAddress()` to accept and parse host parameter
   - Uses `inet_pton()` to convert IP string to binary format
   - Supports `0.0.0.0`, `127.0.0.1`, and specific IPv4 addresses
   - Updated `bindSocket()` and `createAndBindSocket()` to use host

6. **src/server/HttpServer.cpp**
   - Updated `start()` to iterate through host:port pairs
   - Passes host to `createAndBindSocket()`
   - Updated socket tracking to include host information
   - Enhanced logging to show host:port combinations

### New Files Added

1. **conf/host_binding.conf**
   - Example configuration demonstrating host binding
   - Shows various use cases (localhost, specific IP, all interfaces)

2. **docs/HOST_BINDING.md**
   - Comprehensive documentation of the feature
   - Usage examples and testing procedures
   - Error handling and troubleshooting guide

3. **FEATURE_HOST_BINDING_README.md** (this file)
   - Summary of changes and usage

## Usage Examples

### 1. Secure Local Development

```nginx
server {
    listen 127.0.0.1:8080;
    server_name localhost;
    # ... server accessible only from the local machine
}
```

### 2. Multi-Homed Server

```nginx
server {
    listen 10.0.1.100:80;  # Internal network
    server_name internal.example.com;
    # ...
}

server {
    listen 203.0.113.10:80;  # Public network
    server_name www.example.com;
    # ...
}
```

### 3. Development Environment with Multiple Services

```nginx
server {
    listen 127.0.0.1:3000;
    server_name api.local;
    # ... API server
}

server {
    listen 127.0.0.1:4000;
    server_name app.local;
    # ... Application server
}
```

## How to Test

### Build and Run

```bash
# Checkout the branch
git checkout feature/host-ip-binding

# Build the project
make

# Run with example configuration
./webserv conf/host_binding.conf
```

### Test Localhost Binding

```bash
# Terminal 1: Start server
./webserv conf/host_binding.conf

# Terminal 2: Test localhost (should work)
curl http://127.0.0.1:8080/

# Test external IP (should fail if bound to 127.0.0.1)
curl http://$(hostname -I | awk '{print $1}'):8080/
```

### Test Specific IP Binding

```bash
# Find your IP address
ip addr show | grep "inet " | grep -v 127.0.0.1

# Update conf/host_binding.conf with your IP
# Example: listen 192.168.1.50:8080;

# Start server
./webserv conf/host_binding.conf

# Test from same machine
curl http://192.168.1.50:8080/

# Test from another machine on the network
curl http://192.168.1.50:8080/
```

### Verify Socket Binding

```bash
# Check which addresses the server is bound to
netstat -tulpn | grep webserv
# Or
ss -tulpn | grep webserv

# Example output:
tcp   0   0 127.0.0.1:8080    0.0.0.0:*    LISTEN   1234/webserv
tcp   0   0 0.0.0.0:8081      0.0.0.0:*    LISTEN   1234/webserv
```

## Benefits

1. **Security**: Bind to localhost for development to prevent external access
2. **Network Isolation**: Separate internal and external traffic on multi-homed systems
3. **Resource Optimization**: Only listen on required interfaces
4. **Flexibility**: Support various deployment scenarios
5. **NGINX Compatibility**: Follows NGINX configuration syntax

## Backward Compatibility

✅ Fully backward compatible with existing configurations:

- Old configs using `listen 8080;` continue to work
- Defaults to `0.0.0.0` (all interfaces) when host is not specified
- Legacy `getListenPorts()` and `getListenHosts()` methods still available

## Technical Implementation

### Key Data Structures

```cpp
// ServerConfig class
std::vector<std::pair<std::string, int> > _listenAddresses;  // Primary storage
std::vector<std::string> _hosts;  // Backward compatibility
std::vector<int> _ports;          // Backward compatibility

// HttpServer class
struct ServerSocketInfo {
    int socket_fd;
    std::string host;  // New field
    int port;
    size_t serverIndex;
};
```

### Socket Binding Flow

1. **Parse Configuration**: `parseListen()` extracts host and port
2. **Store Addresses**: Save host:port pairs in `_listenAddresses`
3. **Create Sockets**: For each address, call `createAndBindSocket(host, port)`
4. **Convert IP**: Use `inet_pton()` to convert string to binary
5. **Bind Socket**: Call `bind()` with the configured address
6. **Set Non-blocking**: Configure socket for async I/O
7. **Track Socket**: Store in `_serverSockets` with host information

### IP Address Handling

```cpp
// Special cases:
if (host == "0.0.0.0" || host.empty())
    addr->sin_addr.s_addr = htonl(INADDR_ANY);  // All interfaces
else
    inet_pton(AF_INET, host.c_str(), &addr->sin_addr);  // Specific IP
```

## Error Handling

| Error | Cause | Behavior |
|-------|-------|----------|
| Invalid IP | Malformed IP address | Falls back to 0.0.0.0, logs error |
| Bind failure | Port in use or no permission | Skips that address, continues with others |
| Permission denied | Privileged port (<1024) without root | Fails for that socket only |
| Address not available | IP doesn't exist on system | Skips that address |

## Performance Considerations

- No performance overhead compared to previous implementation
- Minimal memory overhead: ~24 bytes per listen address (string + int + pair overhead)
- Socket creation is done once at startup
- No runtime performance impact

## Limitations

1. **IPv4 Only**: Currently supports only IPv4 addresses
2. **No Hostname Resolution**: Must use IP addresses, not hostnames
3. **No IPv6**: No support for IPv6 addresses yet
4. **No Unix Sockets**: No support for Unix domain sockets

## Future Work

Potential enhancements for future iterations:

- [ ] IPv6 support (`listen [::1]:8080;`)
- [ ] Hostname resolution (`listen example.com:8080;`)
- [ ] Unix domain sockets (`listen unix:/tmp/webserv.sock;`)
- [ ] Listen options (`listen 8080 default_server;`, `listen 8080 ssl;`)
- [ ] Automatic IP detection and binding
- [ ] Configuration validation tool

## Testing Checklist

Before merging:

- [x] Localhost binding works (127.0.0.1)
- [x] All interfaces binding works (0.0.0.0)
- [x] Specific IP binding works
- [x] Multiple listen directives work
- [x] Backward compatibility maintained
- [x] Error handling for invalid IPs
- [x] Error handling for bind failures
- [x] Documentation complete
- [ ] Unit tests added (if applicable)
- [ ] Integration tests added (if applicable)

## References

- [NGINX listen directive documentation](https://nginx.org/en/docs/http/ngx_http_core_module.html#listen)
- [BSD socket API - bind(2)](https://man7.org/linux/man-pages/man2/bind.2.html)
- [inet_pton(3)](https://man7.org/linux/man-pages/man3/inet_pton.3.html)
- [RFC 791 - Internet Protocol](https://tools.ietf.org/html/rfc791)

## Support

For questions or issues with this feature:

1. Check `docs/HOST_BINDING.md` for detailed documentation
2. Review example configuration in `conf/host_binding.conf`
3. Open an issue on GitHub with the `feature/host-ip-binding` label

## License

Same as the main webserv project.
