#ifndef EPOLL_THREAD_H
#define EPOLL_THREAD_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include "locker.h"
namespace cachemanager
{
static int sig_pipefd[2];
static int  setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}
/*重置fd上事件，这样操作之后，尽管fd上的EPOLLONESHOT被注册,但是操作系统仍然会触发fd上的EPOLLIN事件，且只触发一次*/
static void reset_oneshot(int epollfd,int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET |EPOLLONESHOT|EPOLLRDHUP|EPOLLHUP|EPOLLERR;
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}
static int setblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL,0);
    int new_option = old_option & ~O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}
static void removefd( int epollfd, int fd )
{
    epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
    close( fd );
}
static void addfd( int epollfd, int fd, bool one_shot )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET |EPOLLRDHUP|EPOLLHUP|EPOLLERR;//| EPOLLERR | EPOLLHUP;
    if ( one_shot )
    {
        event.events |= EPOLLONESHOT;

    }
    epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    setnonblocking( fd );

}

static void modfd( int epollfd, int fd, int ev )
{
    epoll_event  event;
    event.data.fd = fd;
    event.events = ev  | EPOLLET ;
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}
static void addsig( int sig, void( handler )( int ), bool restart = true )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if ( restart )
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}
static void sig_handler( int sig )
{
    int save_errno = errno;
    int msg = sig;
    send( sig_pipefd[1], ( char* )&msg, 1, 0 );
    errno = save_errno;
}
}
#endif