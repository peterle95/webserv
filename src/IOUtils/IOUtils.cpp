#include "IOUtils.hpp"

/*- No direct errno checks remain in application logic near the recv , send , 
read , or write calls.
- All call sites uniformly handle both 0 and -1 returns via IOStatus :
- IO_OK for positive bytes processed.
- IO_WOULD_BLOCK for EAGAIN / EWOULDBLOCK and similar non-fatal cases.
- IO_CLOSED for graceful closures ( recv / read returning 0 ), treated as half-close or EOF.
- IO_ERROR for fatal errors (any other failure).
Loops now properly retry on non-fatal conditions and handle partial operations:
- Client::readRequest continues reading until headers (and optional body by 
Content-Length ) are satisfied.
- Client::writeResponse sends until _response_buffer is exhausted, updating _response_offset .
- CGI::execute writes the entire request body to the CGI stdin pipe, 
advancing data by w.bytes until complete.
- CGI::readResponse reads CGI stdout until EOF, appending chunks to response .*/
static inline bool is_would_block(int e)
{
    return (e == EAGAIN || e == EWOULDBLOCK);
}

IOResult io_recv(int fd, void *buf, size_t len, int flags)
{
    for (;;) {
        ssize_t n = ::recv(fd, buf, len, flags);
        if (n > 0)
            return IOResult(IO_OK, n);
        if (n == 0)
            return IOResult(IO_CLOSED, 0);
        // n == -1
        if (errno == EINTR)
            continue; // retry
        if (is_would_block(errno))
            return IOResult(IO_WOULD_BLOCK, 0);
        return IOResult(IO_ERROR, 0);
    }
}

IOResult io_send(int fd, const void *buf, size_t len, int flags)
{
    for (;;) {
        ssize_t n = ::send(fd, buf, len, flags);
        if (n > 0)
            return IOResult(IO_OK, n);
        if (n == 0)
            return IOResult(IO_WOULD_BLOCK, 0); // treat as would-block/no progress
        // n == -1
        if (errno == EINTR)
            continue; // retry
        if (is_would_block(errno))
            return IOResult(IO_WOULD_BLOCK, 0);
        return IOResult(IO_ERROR, 0);
    }
}

IOResult io_read(int fd, void *buf, size_t len)
{
    for (;;) {
        ssize_t n = ::read(fd, buf, len);
        if (n > 0)
            return IOResult(IO_OK, n);
        if (n == 0)
            return IOResult(IO_CLOSED, 0);
        // n == -1
        if (errno == EINTR)
            continue; // retry
        if (is_would_block(errno))
            return IOResult(IO_WOULD_BLOCK, 0);
        return IOResult(IO_ERROR, 0);
    }
}

IOResult io_write(int fd, const void *buf, size_t len)
{
    for (;;) {
        ssize_t n = ::write(fd, buf, len);
        if (n > 0)
            return IOResult(IO_OK, n);
        if (n == 0)
            return IOResult(IO_WOULD_BLOCK, 0); // no progress
        // n == -1
        if (errno == EINTR)
            continue; // retry
        if (is_would_block(errno))
            return IOResult(IO_WOULD_BLOCK, 0);
        return IOResult(IO_ERROR, 0);
    }
}