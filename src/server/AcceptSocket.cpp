#include "Common.hpp"
#include "HttpServer.hpp"
#include "Client.hpp"

/* Multi-client accept loop using select().
   - Monitors the listening socket for new connections
   - Monitors each client socket based on its state (READING/WRITING)
   - Delegates state transitions to Client::handleConnection()
*/

int HttpServer::runMultiServerAcceptLoop(const std::vector<ServerSocketInfo> &serverSockets)
{

    while (!g_stop)
    {
        fd_set read_fds;
        fd_set write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        int max_fd = -1;

        // Add ALL server sockets to read set (instead of single server_fd)
        for (size_t i = 0; i < serverSockets.size(); ++i)
        {
            int server_fd = serverSockets[i].socket_fd;
            FD_SET(server_fd, &read_fds);
            if (server_fd > max_fd)
                max_fd = server_fd;
        }

        // Add client FDs based on their current state
        for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            int cfd = it->first;
            Client *cl = it->second;
            ClientState st = cl->getState();
            if (st == READING)
            {
                FD_SET(cfd, &read_fds);
                if (cfd > max_fd)
                    max_fd = cfd;
            }
            else if (st == WRITING)
            {
                FD_SET(cfd, &write_fds);
                if (cfd > max_fd)
                    max_fd = cfd;
            }
        }

        struct timeval tv;
        tv.tv_sec = 1; // Periodic timeout to honor shutdown
        tv.tv_usec = 0;

        int ready = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
        if (ready < 0)
        {
            if (errno == EINTR) // Interrupted by signal, retry
                continue;
            std::cerr << "select() failed" << std::endl;
            break;
        }

        // Accept connections from ANY server socket that's ready
        for (size_t i = 0; i < serverSockets.size(); ++i)
        {
            int server_fd = serverSockets[i].socket_fd;

            if (FD_ISSET(server_fd, &read_fds))
            {
                // Accept as many pending connections as possible on this server socket
                while (true)
                {
                    struct sockaddr_in cli;
                    socklen_t clilen = sizeof(cli);
                    int cfd = accept(server_fd, (struct sockaddr *)&cli, &clilen);
                    if (cfd < 0)
                    {
                        if (errno == EINTR) // Interrupted by signal, retry
                            continue;
                        if (errno == EAGAIN || errno == EWOULDBLOCK) // No more connections available
                            break;
                        std::cerr << "accept() failed on server socket " << i << std::endl;
                        break;
                    }

                    if (!HttpServer::setNonBlocking(cfd))
                    {
                        std::cerr << "Failed to set client socket non-blocking" << std::endl;
                        close(cfd);
                        continue;
                    }
                    // response.buildResponse();
                    // Create client with server context information
                    Client *cl = new Client(cfd, *this, serverSockets[i].serverIndex, serverSockets[i].port);
                    _clients[cfd] = cl;

                    DEBUG_PRINT("New connection accepted on server '"
                                << _servers[serverSockets[i].serverIndex].getServerName()
                                << "' port " << serverSockets[i].port << " (fd: " << RED << cfd << RESET << ")");
                }
            }
        }

        // Track clients to close after processing
        std::vector<int> toClose;

        // Process client I/O or generation steps
        for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
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
            std::map<int, Client *>::iterator it = _clients.find(fd);
            if (it != _clients.end())
            {
                close(fd);
                delete it->second;
                _clients.erase(it);
            }
        }
    }

    // Cleanup remaining clients on shutdown
    for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    _clients.clear();

    DEBUG_PRINT("HTTP Server shutting down...");
    return 0;
}
