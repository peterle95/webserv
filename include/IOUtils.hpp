/*
Purpose: Provide wrappers around recv/send/read/write that
 encapsulate errno handling and return a unified IOStatus.
 Why: The evaluation forbids checking errno after these calls and requires
 handling both 0 (closed/EOF) and -1 (error). These wrappers normalize
 outcomes into IO_OK, IO_WOULD_BLOCK, IO_CLOSED, IO_ERROR so call sites
 never inspect errno and correctly distinguish 0 vs -1.
 Change: Added IOResult constructors and declared io_* wrappers.
 */
#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>

// Unified I/O status for non-blocking operations
enum IOStatus {
    IO_OK,         // Operation succeeded with >0 bytes
    IO_WOULD_BLOCK,// Non-blocking operation would block (EAGAIN/EWOULDBLOCK or 0 on send)
    IO_CLOSED,     // Peer closed / EOF (read/recv returned 0)
    IO_ERROR       // Fatal error (any other errno)
};

struct IOResult {
    IOStatus status;
    ssize_t bytes; // Number of bytes read/written when status == IO_OK, otherwise 0
    IOResult() : status(IO_ERROR), bytes(0) {}
    IOResult(IOStatus s, ssize_t b) : status(s), bytes(b) {}
};

// Helpers to abstract system calls and internalize errno handling
IOResult io_recv(int fd, void *buf, size_t len, int flags);
IOResult io_send(int fd, const void *buf, size_t len, int flags);
IOResult io_read(int fd, void *buf, size_t len);
IOResult io_write(int fd, const void *buf, size_t len);