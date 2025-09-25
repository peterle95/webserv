/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 14:20:00 by pmolzer           #+#    #+#             */
/*   Updated: 2025/09/12 15:33:51 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Common.hpp"

HttpServer::HttpServer(int port, const std::string &root, const std::string &index)
: _port(port), _root(root), _index(index) {}

HttpServer::~HttpServer() {}

// read file to string, return empty string on error. Uses binary mode to preserve file contents.
static std::string readFileToString(const std::string &path)
{
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
    if (!ifs.good()) 
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return std::string();
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

// Helper to set a socket to non-blocking mode
static bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return false;
    return true;
}

// Global stop flag set by signal handlers
static volatile sig_atomic_t g_stop = 0;

static void handle_stop_signal(int)
{
    g_stop = 1;
}

// Per-client state for the select()-based event loop
struct ClientState {
    std::string requestBuf;
    std::string responseBuf;
    size_t      sendOffset;
    bool        keepAlive;
    bool        requestComplete;
    HTTPparser  parser;
    ClientState(): sendOffset(0), keepAlive(false), requestComplete(false) {}
};

int HttpServer::start()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "socket() failed" << std::endl;
        return 1;
    }

    // Allow quick restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind address
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)_port);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "bind() failed on port " << _port << std::endl;
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 128) < 0)
    {
        std::cerr << "listen() failed" << std::endl;
        close(server_fd);
        return 1;
    }

    // Set listening socket to non-blocking
    if (!setNonBlocking(server_fd))
    {
        std::cerr << "Failed to set server socket non-blocking" << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "Serving " << _root << "/" << _index << " on http://localhost:" << _port << "/" << std::endl;

    // Signals for graceful shutdown
    std::signal(SIGINT, handle_stop_signal);
    std::signal(SIGTERM, handle_stop_signal);

    // Single-request mode for CI when WEBSERV_ONCE=1
    const char* onceEnv = std::getenv("WEBSERV_ONCE");
    bool serveOnce = (onceEnv && std::string(onceEnv) == "1");
    size_t servedCount = 0;

    std::map<int, ClientState> clients;

    // Master fd sets and bookkeeping
    fd_set master_read_set;
    fd_set master_write_set;
    FD_ZERO(&master_read_set);
    FD_ZERO(&master_write_set);
    FD_SET(server_fd, &master_read_set);
    int max_fd = server_fd;

    // Event loop
    while (true)
    {
        if (g_stop)
            break;

        fd_set read_set = master_read_set;
        fd_set write_set = master_write_set;

        // Optional short timeout to handle signals
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int ready = select(max_fd + 1, &read_set, &write_set, NULL, &tv);
        if (ready < 0)
        {
            if (errno == EINTR) continue;
            std::cerr << "select() failed" << std::endl;
            break;
        }
        if (ready == 0)
        {
            // timeout, loop again to check stop flag
            continue;
        }

        // Iterate over all possible fds up to max_fd
        for (int fd = 0; fd <= max_fd && ready > 0; ++fd)
        {
            // New incoming connections
            if (FD_ISSET(fd, &read_set) && fd == server_fd)
            {
                --ready;
                while (true)
                {
                    struct sockaddr_in cli;
                    socklen_t clilen = sizeof(cli);
                    int cfd = accept(server_fd, (struct sockaddr*)&cli, &clilen);
                    if (cfd < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // No more to accept now
                        if (errno == EINTR)
                            continue;
                        std::cerr << "accept() failed" << std::endl;
                        break;
                    }
                    if (!setNonBlocking(cfd))
                    {
                        close(cfd);
                        continue;
                    }
                    clients[cfd] = ClientState();
                    FD_SET(cfd, &master_read_set);
                    if (cfd > max_fd) max_fd = cfd;
                }
                continue;
            }

            // Readable client socket
            if (FD_ISSET(fd, &read_set) && fd != server_fd)
            {
                --ready;
                std::map<int, ClientState>::iterator it = clients.find(fd);
                if (it == clients.end())
                {
                    // Unknown fd, remove from master sets for safety
                    FD_CLR(fd, &master_read_set);
                    FD_CLR(fd, &master_write_set);
                    continue;
                }
                bool peerClosed = false;
                char buf[4096];
                while (true)
                {
                    ssize_t n = recv(fd, buf, sizeof(buf), 0);
                    if (n > 0)
                    {
                        it->second.requestBuf.append(buf, buf + n);
                        // Heuristic: consider request complete if we reached end of headers
                        if (!it->second.requestComplete)
                        {
                            if (it->second.requestBuf.find("\r\n\r\n") != std::string::npos)
                            {
                                it->second.requestComplete = true;
                            }
                        }
                        // Continue reading until EAGAIN
                        continue;
                    }
                    else if (n == 0)
                    {
                        peerClosed = true;
                        break;
                    }
                    else
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        if (errno == EINTR)
                            continue;
                        // Fatal recv error
                        peerClosed = true;
                        break;
                    }
                }

                if (peerClosed)
                {
                    close(fd);
                    FD_CLR(fd, &master_read_set);
                    FD_CLR(fd, &master_write_set);
                    clients.erase(fd);
                    continue;
                }

                // If request parsing is complete, generate a response and switch to write
                if (it->second.requestComplete && it->second.responseBuf.empty())
                {
                    it->second.parser.parseRequest(it->second.requestBuf);

                    std::string path = it->second.parser.getPath();
                    std::string filePath;
                    if (path == "/" || path.empty())
                        filePath = _root + "/" + _index;
                    else
                    {
                        if (path.size() > 0 && path[0] == '/')
                            filePath = _root + path;
                        else
                            filePath = _root + "/" + path;
                    }

                    std::string body = readFileToString(filePath);
                    std::ostringstream resp;
                    // Determine keep-alive based on request headers/version
                    std::string conn = it->second.parser.getHeader("Connection");
                    std::string version = it->second.parser.getVersion();
                    // Normalize header value to lowercase for comparison
                    for (size_t i = 0; i < conn.size(); ++i) conn[i] = static_cast<char>(std::tolower(conn[i]));
                    bool wantKeepAlive = false;
                    if (!conn.empty())
                        wantKeepAlive = (conn == "keep-alive");
                    else if (version == "HTTP/1.1")
                        wantKeepAlive = true; // default keep-alive for HTTP/1.1

                    if (!body.empty())
                    {
                        resp << "HTTP/1.1 200 OK\r\n";
                        resp << "Content-Type: text/html; charset=utf-8\r\n";
                        resp << "Content-Length: " << body.size() << "\r\n";
                        if (wantKeepAlive) resp << "Connection: keep-alive\r\n"; else resp << "Connection: close\r\n";
                        resp << "\r\n";
                        resp << body;
                    }
                    else
                    {
                        std::string notFound = "<h1>404 Not Found</h1>";
                        resp << "HTTP/1.1 404 Not Found\r\n";
                        resp << "Content-Type: text/html; charset=utf-8\r\n";
                        resp << "Content-Length: " << notFound.size() << "\r\n";
                        if (wantKeepAlive) resp << "Connection: keep-alive\r\n"; else resp << "Connection: close\r\n";
                        resp << "\r\n";
                        resp << notFound;
                    }

                    it->second.responseBuf = resp.str();
                    it->second.sendOffset = 0;
                    it->second.keepAlive = wantKeepAlive;

                    // Move fd from read to write set
                    FD_CLR(fd, &master_read_set);
                    FD_SET(fd, &master_write_set);

                    if (serveOnce)
                        ++servedCount;
                }
                continue;
            }

            // Writable client socket
            if (FD_ISSET(fd, &write_set) && fd != server_fd)
            {
                --ready;
                std::map<int, ClientState>::iterator it = clients.find(fd);
                if (it == clients.end())
                {
                    FD_CLR(fd, &master_write_set);
                    continue;
                }

                const std::string &resp = it->second.responseBuf;
                while (it->second.sendOffset < resp.size())
                {
                    ssize_t n = send(fd, resp.c_str() + it->second.sendOffset, resp.size() - it->second.sendOffset, 0);
                    if (n > 0)
                    {
                        it->second.sendOffset += (size_t)n;
                        continue;
                    }
                    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                    {
                        break; // Can't send more now
                    }
                    if (n < 0 && errno == EINTR)
                    {
                        continue; // retry
                    }
                    // Fatal error or peer closed
                    it->second.sendOffset = resp.size();
                    break;
                }

                if (it->second.sendOffset >= resp.size())
                {
                    if (it->second.keepAlive)
                    {
                        // Reset for next request, go back to reading
                        it->second.requestBuf.clear();
                        it->second.responseBuf.clear();
                        it->second.sendOffset = 0;
                        it->second.requestComplete = false;
                        FD_CLR(fd, &master_write_set);
                        FD_SET(fd, &master_read_set);
                    }
                    else
                    {
                        // Close connection
                        close(fd);
                        FD_CLR(fd, &master_write_set);
                        clients.erase(fd);
                    }
                }
                continue;
            }
        }

        // In CI single-request mode, exit after serving one complete response
        if (serveOnce && servedCount >= 1)
            break;
    }

    // Cleanup
    for (std::map<int, ClientState>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        close(it->first);
    }
    close(server_fd);
    return 0;
}
