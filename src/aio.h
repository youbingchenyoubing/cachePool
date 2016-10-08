#ifndef AIO_H
#define AIO_H
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <linux/aio_abi.h>
namespace cachemanager
{
/* centos 7 version __NR_io_setup都是对应到头文件/usr/include/asm/unistd_64.h 64位系统为例*/
static inline int io_setup( unsigned nr_events, aio_context_t* ctx_idp )
{
    return syscall( __NR_io_setup, nr_events, ctx_idp );
}
static inline int io_destroy( aio_context_t ctx )
{
    return syscall( __NR_io_destroy, ctx );
}
static inline int io_submit( aio_context_t ctx, long nr, struct iocb** iocbpp )
{

    return syscall( __NR_io_submit, ctx, nr, iocbpp );
}
static inline int io_getevents( aio_context_t ctx, long min_nr, long nr, struct io_event* events, struct timespec*   timeout )
{
    return syscall( __NR_io_getevents, ctx, min_nr, nr, events, timeout );
}

static inline int io_cancel( aio_context_t ctx, struct iocb* iocb, struct io_event* result )
{
    return  syscall( __NR_io_cancel, ctx, iocb, result );
}

/*
 * We call io_setup(), io_destroy() io_submit(), and io_getevents() directly
 * as syscalls instead of libaio usage, because the library header file
 * suppo	rts eventfd() since 0.3.107 version only.
 */

}
#endif