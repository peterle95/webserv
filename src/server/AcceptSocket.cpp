#include "Common.hpp"
#include "HttpServer.hpp"
#include "Client.hpp"

/* Multi-client accept loop using select().
   - Monitors the listening socket for new connections
   - Monitors each client socket based on its state (READING/WRITING)
   - Delegates state transitions to Client::handleConnection()
*/
int HttpServer::runAcceptLoop(int server_fd, bool serveOnce)
{
    size_t acceptedCount = 0;

    while (!g_stop)
    {
        fd_set read_fds;
        fd_set write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        int max_fd = server_fd;
        FD_SET(server_fd, &read_fds);

        // Add client FDs based on their current state
        for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            int cfd = it->first;
            Client *cl = it->second;
            ClientState st = cl->getState();
            if (st == READING)
            {
                FD_SET(cfd, &read_fds);
                if (cfd > max_fd) max_fd = cfd;
            }
            else if (st == WRITING)
            {
                FD_SET(cfd, &write_fds);
                if (cfd > max_fd) max_fd = cfd;
            }
            // GENERATING_RESPONSE will be handled post-select without waiting on FDs
            // AWAITING_CGI not used in current synchronous CGI path
        }

        struct timeval tv;
        tv.tv_sec = 1; // Periodic timeout to honor shutdown
        tv.tv_usec = 0;

        int ready = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            std::cerr << "select() failed" << std::endl;
            break;
        }

        // Accept as many pending connections as possible
        if (FD_ISSET(server_fd, &read_fds))
        {
            while (true)
            {
                struct sockaddr_in cli;
                socklen_t clilen = sizeof(cli);
                int cfd = accept(server_fd, (struct sockaddr*)&cli, &clilen);
                if (cfd < 0)
                {
                    if (errno == EINTR)
                        continue;
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break;
                    std::cerr << "accept() failed" << std::endl;
                    break;
                }

                if (!HttpServer::setNonBlocking(cfd))
                {
                    std::cerr << "Failed to set client socket non-blocking" << std::endl;
                    close(cfd);
                    continue;
                }

                Client *cl = new Client(cfd, *this);
                _clients[cfd] = cl;
                ++acceptedCount;
                DEBUG_PRINT("New connection accepted (fd: " << RED << cfd << RESET << ")");
            }
        }

        // Track clients to close after processing
        std::vector<int> toClose;

        // Process client I/O or generation steps
        for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            int cfd = it->first;
            Client *cl = it->second;
            ClientState st = cl->getState();

            if (st == GENERATING_RESPONSE)
            {
                cl->handleConnection();
            }
            else if (st == READING && FD_ISSET(cfd, &read_fds))
            {
                cl->handleConnection();
            }
            else if (st == WRITING && FD_ISSET(cfd, &write_fds))
            {
                cl->handleConnection();
            }
            else if (st == AWAITING_CGI)
            {
                cl->handleConnection();
            }

            if (cl->getState() == CLOSING)
            {
                toClose.push_back(cfd);
            }
        }

        // Close and delete clients marked for closing
        for (size_t i = 0; i < toClose.size(); ++i)
        {
            int fd = toClose[i];
            std::map<int, Client*>::iterator it = _clients.find(fd);
            if (it != _clients.end())
            {
                close(fd);
                delete it->second;
                _clients.erase(it);
            }
        }

        // Serve-once mode: stop after serving a single connection when no clients remain active
        if (serveOnce && acceptedCount >= 1 && _clients.empty())
            break;
    }

    // Cleanup remaining clients on shutdown
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    _clients.clear();

    DEBUG_PRINT("HTTP Server shutting down...");
    return 0;
}
