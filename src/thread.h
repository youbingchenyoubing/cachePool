//线程池类
#ifndef THREAD_H
#define THREAD_H
# include <iostream>
# include <list>
# include <cstdio>
# include <exception>
# include <pthread.h>
# include "locker.h"
#ifdef __LOG_DEBUG
# include "./Log/Log.h"
#endif
using namespace std;
namespace cachemanager
{
/*线程类，应该是所有的客户请求是无状态的，因为同一连接上的不同请求可能由不同线程处理*/
template < typename T>
class ThreadManager
{
public:
    static ThreadManager<T>* getInstance ( unsigned int threadNumber, unsigned int maxRequests );
    ~ThreadManager();
    bool append( T* request );
private:
    static void* worker( void* arg ); //线程调用的函数，必须是一个静态成员函数
    void run();
    ThreadManager( unsigned int threadNumber, unsigned int maxRequests = 10000 );
private:

    int m_thread_number;
    int m_max_requests;
    bool m_stop;
    pthread_t* m_pthreads;
    std::list<T*> m_workqueue;
    locker m_queuelocker;
    sem m_queuestat;
    static ThreadManager <T>*  m_instance;
};
template <typename T>
ThreadManager<T>* ThreadManager <T>::m_instance = nullptr;
template <typename T>
ThreadManager<T>::ThreadManager( unsigned int threadNumber, unsigned int maxRequests ):
    m_thread_number( threadNumber ), m_max_requests( maxRequests ), m_stop( false ), m_pthreads( nullptr )
{

    if ( ( threadNumber <= 0 ) || ( maxRequests <= 0 ) )
    {
        throw std::exception();
    }
    m_pthreads = new pthread_t[m_thread_number];
    if ( !m_pthreads )
    {
        throw std::exception();
    }
    for ( int i = 0; i < threadNumber; ++i )
    {

        cout << "create the " << i + 1 << "th thread" << endl;
        if ( pthread_create( m_pthreads + i, NULL, worker, this ) != 0 )
        {
            if ( m_pthreads )
            {
                delete [] m_pthreads;
                throw std::exception();
                m_pthreads = nullptr;
            }
        }
        if ( pthread_detach( m_pthreads[i] ) )
        {
            if ( m_pthreads )
            {
                delete [] m_pthreads;
                throw std::exception();
                m_pthreads = nullptr;
            }
        }
    } // end  for (int i = 0;i< thread_number;++i)
}// end ThreadManager Construct
template <typename T>
ThreadManager<T>* ThreadManager<T>::getInstance ( unsigned int threadNumber, unsigned int maxRequests = 10000 )
{
    if ( !m_instance )
    {
        m_instance = new ThreadManager<T>( threadNumber, maxRequests );
    }
    return  m_instance;
}
template <typename T>
ThreadManager<T>::~ThreadManager()
{
    if ( m_pthreads )
    {
        delete [] m_pthreads;
        m_stop = true;
        m_pthreads = nullptr;
    }
}

template <typename T>
bool ThreadManager<T>::append( T* request )
{
    m_queuelocker.lock();
    if ( m_workqueue.size()  > m_max_requests )
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back( request );
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
template <typename T>
void* ThreadManager<T>::worker( void* arg )
{
    ThreadManager* pool = ( ThreadManager* ) arg;

    pool->run();

    return pool;
}

template <typename T>
void ThreadManager<T>::run()
{
    while ( !m_stop )
    {
        m_queuestat.wait();  // 信号量一定要在锁前面
        m_queuelocker.lock();
        if ( m_workqueue.empty() )
        {
            m_queuelocker.unlock();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        //LOG_ERROR(__FILE__,__LINE__,"还有多少个事件没处理:%d",m_workqueue.size());
        m_queuelocker.unlock();
        if ( !request )
        {
            continue;
        }

        request->process();


    }//end while

}
}
#endif