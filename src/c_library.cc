#include <errno.h>
#include <stdarg.h>

#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "async.h"
#include "loop.h"


siren::Loop *siren_loop = nullptr;
siren::Async *siren_async = nullptr;


extern "C" {

int
siren_open(const char *arg1, int arg2, ...) noexcept
{
    va_list ap;
    va_start(ap, arg2);
    mode_t arg3 = va_arg(ap, mode_t);
    va_end(ap);
    return siren_loop->open(arg1, arg2, arg3);
}


int
siren_fs_open(const char *arg1, int arg2, ...) noexcept
{
    va_list ap;
    va_start(ap, arg2);
    mode_t arg3 = va_arg(ap, mode_t);
    va_end(ap);

    try {
        return siren_async->callFunction(open, arg1, arg2, arg3);
    } catch (siren::FiberInterruption){
        errno = ECANCELED;
        return -1;
    }
}


int
siren_fcntl(int arg1, int arg2, ...) noexcept
{
    va_list ap;
    va_start(ap, arg2);
    int arg3 = va_arg(ap, int);
    va_end(ap);
    return siren_loop->fcntl(arg1, arg2, arg3);
}


int
siren_pipe(int arg1[2]) noexcept
{
    return siren_loop->pipe(arg1);
}


int
siren_pipe2(int arg1[2], int arg2) noexcept
{
    return siren_loop->pipe2(arg1, arg2);
}


ssize_t
siren_read(int arg1, void *arg2, size_t arg3) noexcept
{
    try {
        return siren_loop->read(arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_fs_read(int arg1, void *arg2, size_t arg3) noexcept
{
    try {
        return siren_async->callFunction(read, arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_write(int arg1, const void *arg2, size_t arg3) noexcept
{
    try {
        return siren_loop->write(arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_fs_write(int arg1, const void *arg2, size_t arg3) noexcept
{
    try {
        return siren_async->callFunction(write, arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_readv(int arg1, const struct iovec *arg2, int arg3) noexcept
{
    try {
        return siren_loop->readv(arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_fs_readv(int arg1, const struct iovec *arg2, int arg3) noexcept
{
    try {
        return siren_async->callFunction(readv, arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_writev(int arg1, const struct iovec *arg2, int arg3) noexcept
{
    try {
        return siren_loop->writev(arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_fs_writev(int arg1, const struct iovec *arg2, int arg3) noexcept
{
    try {
        return siren_async->callFunction(writev, arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


off_t
siren_lseek(int arg1, off_t arg2, int arg3) noexcept
{
    try {
        return siren_async->callFunction(lseek, arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


int
siren_close(int arg1) noexcept
{
    return siren_loop->close(arg1);
}


int
siren_fs_close(int arg1) noexcept
{
    try {
        return siren_async->callFunction(close, arg1);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


int
siren_usleep(useconds_t arg1) noexcept
{
    try {
        return siren_loop->usleep(arg1);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


int
siren_socket(int arg1, int arg2, int arg3) noexcept
{
    return siren_loop->socket(arg1, arg2, arg3);
}


int
siren_getsockopt(int arg1, int arg2, int arg3, void *arg4, socklen_t *arg5) noexcept
{
    return siren_loop->getsockopt(arg1, arg2, arg3, arg4, arg5);
}


int
siren_setsockopt(int arg1, int arg2, int arg3, const void *arg4, socklen_t arg5) noexcept
{
    return siren_loop->setsockopt(arg1, arg2, arg3, arg4, arg5);
}


int
siren_accept(int arg1, struct sockaddr *arg2, socklen_t *arg3) noexcept
{
    try {
        return siren_loop->accept(arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


int
siren_accept4(int arg1, struct sockaddr *arg2, socklen_t *arg3, int arg4) noexcept
{
    try {
        return siren_loop->accept4(arg1, arg2, arg3, arg4);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


int
siren_connect(int arg1, const struct sockaddr *arg2, socklen_t arg3) noexcept
{
    try {
        return siren_loop->connect(arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_recv(int arg1, void *arg2, size_t arg3, int arg4) noexcept
{
    try {
        return siren_loop->recv(arg1, arg2, arg3, arg4);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_send(int arg1, const void *arg2, size_t arg3, int arg4) noexcept
{
    try {
        return siren_loop->send(arg1, arg2, arg3, arg4);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_recvfrom(int arg1, void *arg2, size_t arg3, int arg4, struct sockaddr *arg5
               , socklen_t *arg6) noexcept
{
    try {
        return siren_loop->recvfrom(arg1, arg2, arg3, arg4, arg5, arg6);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


ssize_t
siren_sendto(int arg1, const void *arg2, size_t arg3, int arg4, const struct sockaddr *arg5
             , socklen_t arg6) noexcept
{
    try {
        return siren_loop->sendto(arg1, arg2, arg3, arg4, arg5, arg6);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}


int
siren_getaddrinfo(const char *arg1, const char *arg2, const struct addrinfo *arg3
                  , struct addrinfo **arg4) noexcept
{
    try {
        return siren_async->callFunction(getaddrinfo, arg1, arg2, arg3, arg4);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return EAI_SYSTEM;
    }
}


int
siren_getnameinfo(const struct sockaddr *arg1, socklen_t arg2, char *arg3, socklen_t arg4
                  , char *arg5, socklen_t arg6, int arg7) noexcept
{
    try {
        return siren_async->callFunction(getnameinfo, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return EAI_SYSTEM;
    }
}


int
siren_poll(struct pollfd *arg1, nfds_t arg2, int arg3) noexcept
{
    try {
        return siren_loop->poll(arg1, arg2, arg3);
    } catch (siren::FiberInterruption) {
        errno = ECANCELED;
        return -1;
    }
}

} // extern "C"
