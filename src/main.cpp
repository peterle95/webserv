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

static bool setServeOnce()
{
    return setenv("WEBSERV_ONCE", "1", 1) == 0;
}

static pid_t spawnServerOnce(const std::string &configPath)
{
    pid_t pid = fork();
    if (pid < 0)
        return -1;

    if (pid == 0)
    {
        // Child: start the server and serve a single connection
        setServeOnce();
        ConfigParser parserChild;
        if (!parserChild.parse(configPath))
            _exit(2);
        HttpServer serverChild(parserChild);
        int rc = serverChild.start();
        _exit(rc);
    }
    // Parent
    return pid;
}

static int connectWithRetry(int port, int maxAttempts, int sleepMicros)
{
    int fd = -1;
    for (int attempt = 0; attempt < maxAttempts; ++attempt)
    {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            return -1;
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons((uint16_t)port);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0)
            return fd;
        close(fd);
        usleep(sleepMicros);
    }
    return -1;
}

static bool sendAll(int fd, const std::string &data)
{
    size_t off = 0;
    while (off < data.size())
    {
        ssize_t n = send(fd, data.c_str() + off, data.size() - off, 0);
        if (n > 0)
        {
            off += (size_t)n;
            continue;
        }
        if (n < 0 && (errno == EINTR))
            continue;
        return false;
    }
    return true;
}

static std::string recvAll(int fd)
{
    std::string out;
    char buf[4096];
    while (true)
    {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n > 0)
            out.append(buf, buf + n);
        else if (n == 0)
            break;
        else if (n < 0)
            continue;
        else
            break;
    }
    return out;
}

static bool assertContains(const std::string &hay, const std::string &needle)
{
    return hay.find(needle) != std::string::npos;
}

static bool runSingleRequestTest(const std::string &configPath, int port,
                                 const std::string &request,
                                 const std::string &expect)
{
    pid_t srv = spawnServerOnce(configPath);
    if (srv < 0)
    {
        std::cerr << "Failed to spawn server" << std::endl;
        return false;
    }

    int cfd = connectWithRetry(port, 50, 50 * 1000); // up to ~2.5s
    if (cfd < 0)
    {
        std::cerr << "Client failed to connect" << std::endl;
        int status;
        waitpid(srv, &status, 0);
        return false;
    }

    bool okSend = sendAll(cfd, request);
    shutdown(cfd, SHUT_WR);
    std::string resp = recvAll(cfd);
    close(cfd);

    int status;
    waitpid(srv, &status, 0);

    if (!okSend)
    {
        std::cerr << "Send failed" << std::endl;
        return false;
    }

    if (!assertContains(resp, expect))
    {
        std::cerr << "Unexpected response. Expected to contain: '" << expect << "'\n";
        std::cerr << "Response was:\n"
                  << resp << std::endl;
        return false;
    }
    return true;
}

static int runTests(const std::string &configPath)
{
    ConfigParser parser;
    if (!parser.parse(configPath))
        return 1;
    // Get first port from first server
    const std::vector<ServerConfig> &servers = parser.getServers();
    int port = 8080; // Default
    if (!servers.empty() && !servers[0].getListenPorts().empty())
        port = servers[0].getListenPorts()[0];

    int failures = 0;

    // Test 1: GET /
    {
        std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        if (!runSingleRequestTest(configPath, port, req, "HTTP/1.1 200"))
            ++failures;
    }

    // Test 2: Bad Request (malformed request line)
    {
        std::string req = "GARBAGE\r\n\r\n";
        if (!runSingleRequestTest(configPath, port, req, "HTTP/1.1 400"))
            ++failures;
    }

    // Test 3: Method Not Allowed (PUT)
    {
        std::string req = "PUT / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        if (!runSingleRequestTest(configPath, port, req, "HTTP/1.1 405"))
            ++failures;
    }

    std::cout << "Tests completed. Failures: " << failures << std::endl;
    return failures == 0 ? 0 : 1;
}

// implement later try...catch blocks
int main(int argc, char **argv)
{
    // Mode selection
    // Usage:
    //   ./webserv                 -> normal server
    //   ./webserv --run-tests     -> run basic client/response tests

    std::string configPath = "conf/default.conf";

    if (argc >= 2 && std::string(argv[1]) == "--run-tests")
    {
        if (argc >= 3)
            configPath = argv[2];
        return runTests(configPath);
    }

    if (argc == 2)
        configPath = argv[1];
    else if (argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " [--run-tests] [conf/config_file.conf]" << std::endl;
        return 1;
    }

    ConfigParser parser;
    if (!parser.parse(configPath))
        return 1;
    HttpServer server(parser);
    return server.start();
}
