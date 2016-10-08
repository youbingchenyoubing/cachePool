// 进程池类
#ifndef PROCESS_H
#define PROCESS_H
# include "epoll.h"
# include <iostream>
using namespace std;
//子类进程
struct processSun
{
    pid_t m_pid;
    int m_pipefd[2];// 父进程和 子进程通信信用通道

    processSun()
    {
        m_pid = -1;
    }
};
template < typename T>
class ProcessManager
{
private:
    ProcessManager( int listenfd, unsigned int processNumber );

public:
    static ProcessManager <T>* getInstance( int listenfd, unsigned int processNumber );
    // 进程池接口，启动进程池
    ~ProcessManager()
    {
        if ( m_sub_process )
        {
            delete [] m_sub_process;

            m_sub_process = NULL;
        }
    }
    void run();
private:
    void setup_sig_pipe();
    void run_parent();
    void run_child();

private:
    /*进程池允许的最大子进程数量*/
    static const unsigned int MAX_PROCESS_NUMBER = 16;
    /*每个子进程最多处理的客户数量*/
    static const unsigned int USER_PER_PROCESS = 65536;
    /*epoll最多 处理的事件数 */
    static const  unsigned int MAX_EVENT_NUMBER = 1000;
    /*进程池中的进程总数*/
    unsigned int m_process_number;
    /*子进程在进程池中的序号，从0开始*/
    int m_idx;
    /*每个进程都有一个epoll内核事件，用m_epollfd标识*/
    int m_epollfd;
    /*监听socket*/
    int m_listenfd;
    /*子进程通过m_stop来决定是否停止运行*/
    int m_stop;
    /*保存所有子进程描述状态*/
    processSun* m_sub_process;
    /*进程池静态实例*/
    static ProcessManager<T>* m_instance;
};
template <typename T>
ProcessManager <T>* ProcessManager<T>::m_instance = NULL;
template <typename T>
ProcessManager<T>::ProcessManager( int listenfd, unsigned int processNumber )
    : m_listenfd( listenfd ), m_process_number( processNumber ), m_idx( -1 ), m_stop( false )
{
    assert( ( processNumber > 0 ) && ( processNumber <= MAX_PROCESS_NUMBER ) );

    m_sub_process = new processSun[processNumber];

    /*创建process_number个进程，并建立它们与父进程之间的 关系*/

    for ( int  i = 0; i < processNumber; ++i )
    {
        int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd ); //实现父子进程 间通信

        assert( ret == 0 );
        m_sub_process[i].m_pid = fork();
        assert( m_sub_process[i].m_pid >= 0 );
        if ( m_sub_process[i].m_pid > 0 ) //子进程
        {
            close( m_sub_process[i].m_pipefd[1] );

            continue;
        }

        else //父进程
        {
            close( m_sub_process[i].m_pipefd[0] );

            m_idx = i;

            break;
        }
    }
}

template <typename T>
ProcessManager <T>* ProcessManager<T>::getInstance( int listenfd, unsigned int processNumber )
{
    if ( !m_instance )
    {
        m_instance = new ProcessManager<T>( listenfd, processNumber );
    }
    return   m_instance;
}
/*统一事件源*/
template <typename T>
void ProcessManager<T>::setup_sig_pipe()
{

    m_epollfd = epoll_create( 5 );

    assert( m_epollfd != -1 );


    int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, sig_pipefd );

    assert( ret != -1 );

    setnonblocking( sig_pipefd[1] );

    addfd( m_epollfd, sig_pipefd[0] );

    addsig( SIGCHLD, sig_handler );
    addsig( SIGTERM, sig_handler );

    addsig( SIGINT, sig_handler );

    addsig( SIGPIPE, SIG_IGN );

}
template <typename T>
void ProcessManager<T>::run()
{
    if ( m_idx != -1 )
    {
        run_child();

        return;
    }
    run_parent();
}
template <typename T>
void ProcessManager<T>::run_child()
{
    setup_sig_pipe();

    int pipefd = m_sub_process[m_idx].m_pipefd[1];

    addfd( m_epollfd, pipefd );

    epoll_event events [MAX_EVENT_NUMBER];

    T* users = new T [USER_PER_PROCESS];

    assert( users );

    int number = 0;

    int ret = -1;

    while ( !m_stop )
    {
        number = epoll_wait( m_epollfd, events, MAX_EVENT_NUMBER, -1 );

        if ( ( number < 0 ) && ( errno != EINTR ) )
        {
            cout << "epoll failure\n";
            break;
        }
        for ( int i = 0; i < number; ++i )
        {
            int sockfd = events[i].data.fd;

            if ( ( sockfd == pipefd ) && ( events[i].events & EPOLLIN ) ) //管道有数据读出
            {
                int client = 0;
                ret = recv( sockfd, ( char* ) &client, sizeof( client ), 0 );

                if ( ( ret < 0 ) && ( errno != EAGAIN ) || ret == 0 )
                {
                    continue;
                }

                else
                {

                    struct sockaddr_in client_address;

                    socklen_t client_addrlength = sizeof( client_address );

                    int connfd = accept( m_listenfd, ( struct sockaddr* )&client_address, &client_addrlength );

                    if ( connfd < 0 )
                    {
                        cout << "errno is " << errno << endl;
                        continue;
                    }
                    addfd( m_epollfd, connfd );
                    users[connfd].init( m_epollfd, connfd, client_address );


                } //end  else
            }//end if((sockfd == pipefd) && (events[i].events & EPOLLIN))
            // 子进程处理信号
            else if ( ( sockfd == sig_pipefd[0] ) && ( events[i].events & EPOLLIN ) )
            {

                int sig;
                char signals[1024];

                ret  = recv( sig_pipefd[0], signals, sizeof( signals ), 0 );

                if ( ret <= 0 )
                {
                    continue;
                }

                else
                {
                    for (  int i = 0; i < ret; ++i )
                    {
                        switch ( signals[i] )
                        {
                        case SIGCHLD:
                        {
                            pid_t pid;
                            int stat;
                            while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 )
                            {
                                continue;
                            } // end while
                            break;
                        } //end case SIGCHLD
                        case SIGTERM:
                        case SIGINT:
                        {
                            m_stop = true;

                            break;
                        }
                        default:
                            break;
                        }//end switch(signals[i])
                    }// end for( int i =0;i<ret;++i)
                }// end else

            }// end else if((sockfd == sig_pipefd[0])&&(events[i].events & EPOLLIN))

            else if ( events[i].events & EPOLLIN )
            {
                users[sockfd].process();
            }
            else
            {
                continue;
            }
        }//end for
    }//end while
    delete [] users;
    users = NULL;
    close( pipefd );
    close( m_epollfd );
}// end runchild

template <typename T>
void  ProcessManager<T>::run_parent()
{
    setup_sig_pipe();

    addfd( m_epollfd, m_listenfd );

    epoll_event events[MAX_EVENT_NUMBER];

    int sub_process_counter = 0;
    int new_conn = 1;
    int number = 0;
    int ret = -1;
    while ( ! m_stop )
    {
        number =  epoll_wait( m_epollfd, events, MAX_EVENT_NUMBER, -1 );

        if ( ( number < 0 ) && ( errno != EINTR ) )
        {
            cout << "epoll failure" << endl;

            break;
        }

        for ( int i = 0; i < number; ++i )
        {
            int sockfd = events[i].data.fd;

            if ( sockfd == m_listenfd )
            {
                int i = sub_process_counter;
                do
                {
                    if ( m_sub_process[i].m_pid != -1 )
                    {
                        break;
                    }
                    i = ( i + 1 ) % m_process_number;
                }
                while ( i != sub_process_counter ); //采用Round Robin算法
                if ( m_sub_process[i].m_pid == -1 )
                {
                    m_stop = true;

                    break;
                }
                sub_process_counter = ( i + 1 ) % m_process_number;

                send( m_sub_process[i].m_pipefd[0], ( char* )&new_conn, sizeof( new_conn ), 0 );

                cout << "send request  to  child \n" << i;

            }// end if( sockfd == m_listenfd)

            else if ( ( sockfd == sig_pipefd[0] ) && ( events[i].events & EPOLLIN ) )
            {
                int sig;
                char signals[1024];
                ret = recv( sig_pipefd[0], signals, sizeof( signals ), 0 );
                if ( ret <= 0 )
                {
                    continue;
                }
                else
                {
                    for ( int i = 0; i < ret; ++i )
                    {
                        switch ( signals[i] )
                        {
                        case SIGCHLD:
                        {
                            pid_t pid;

                            int stat;

                            while ( ( pid == waitpid( -1, &stat, WNOHANG ) ) > 0 )
                            {
                                for ( int  i = 0; i < m_process_number; ++i )
                                {
                                    if ( m_sub_process[i].m_pid == pid )
                                    {
                                        cout << "child " << i << "join\n";
                                        close( m_sub_process[i].m_pipefd[0] );
                                        m_sub_process[i].m_pid = -1;
                                    }
                                }//  end for
                            }// end  while((pid == waitpid(-1,&stat,WNOHANG))>0)
                            m_stop = true;
                            for ( int i = 0; i < m_process_number; ++i )
                            {
                                if ( m_sub_process[i].m_pid != -1 )
                                {
                                    m_stop = false;
                                }
                            }// end  for (int i = 0;i<m_process_number;++i)
                            break;
                        }
                        case SIGTERM:
                        case SIGINT:
                        {
                            cout << "kill all the child now \n";
                            for ( int i = 0; i < m_process_number; ++i )
                            {
                                int pid = m_sub_process[i].m_pid;

                                if ( pid != -1 )
                                {
                                    kill( pid, SIGTERM );
                                }
                            }
                            break;
                        }
                        default:
                        {
                            break;
                        }
                        }//end switch(signals[i])
                    } // end for( int i = 0;i<ret;++i)

                }// end else
            } // end  else if((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN))
            else
            {
                continue;
            }

        }//end  for( int i = 0;i<number; ++i)
    }//end  while(! m_stop)
    close( m_epollfd );
}
#endif