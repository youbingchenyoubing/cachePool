#ifndef ENTRANCE_H
#define ENTRANCE_H
# include "parsehash.h"
# include "writeDisk.h"
# include "thread.h"
# include "bitmap.h"
#ifdef __LOG_DEBUG
# include "./Log/Log.h"
#endif
# include "similarlinkedhashmap.h" //管理资源
//# include "process.h" //进程池
//# include "thread.h"
//# include "locker.h"
# include "socket.h"
//线程池
# include "parsexml.h"
# include "protocol.h"
# include "epoll_thread.h"
# include <string.h>
# include <string>
# include <netinet/in.h>
# define  MAX_FD 65536
# define  MAX_EVENT_NUMBER 100000
using namespace cachemanager;
struct ClientRequest
{
    //string  fileName;   //文件名
    char* fileName;
    unsigned int offset; // 偏移量

    unsigned int len; // 请求长度

    //unsigned int beginOffset; // 请求数据第一块开始的偏移量，不等于上面的偏移量 ,这个是要根据，cache配置进行计算的

    //unsigned int endOffset; // 请求数据最后一块开始的偏移量。这个是要根据，cache配置进行计算的

    //void  *data;    //  如果是读事件就是为空

    char* type;    //表明用户是读、写事件

    //char * flag;   //如果是为真的，即使缓存没有也要立即从磁盘上读IO
    //char  *ip; // 客户端的ip地址

    //unsigned short int port; //客户端的端口号

    //bool done; //用来标示是否处理完这个事件
    ClientRequest()
    {

    }

};
// 配置文件类

/*用于处理客户的请求类*/
class CacheConn
{
public:
    CacheConn()
    {
        //clientData == NULL;
        m_write = nullptr;
        data = nullptr;
    }
    ~CacheConn()
    {
        if(m_write!=nullptr)
        {
            delete [] m_write;
            m_write = nullptr;
        }
        if(data!=nullptr)
        {
            delete data;
            data=nullptr;
        }
    }
    //void init(int fd,)
    void close_conn(int type);
    void init( int sockfd, const sockaddr_in& client_addr );
    void process();
    bool read();
    static int m_epollfd;
    static int m_user_count;//统计用户数量
private:
    static const int BUFFER_SIZE = 1024;  //缓存区的大小
    //static const unsigned int W_DATA = 1024;   //存储客户写数据的大小

    char* m_write;  // 应用层缓存区
    LinkListNode*   data; //用来暂时存储读数据
    sockaddr_in m_address;
    int m_sockfd;
    char m_buf[BUFFER_SIZE];
    int m_read_idx; //标志读缓存区中已经读入客户数据的最后一个字节的下一个位置
    unsigned int m_write_idx; //写索引
    ClientRequest clientData; //存储客户端的协议
    //ClientRequest *clientData;
private:
    void init(); //初始化其他变量
    //void printftest();//打印解析后文件，仅仅测试使用
    bool ParseRequest();
    //void writeDisk2();//处理客户端的写事件,兼容定长或不定长
    void ResponesClient( unsigned int code, void* data = NULL );
    BitMap *  rebuildBitMap(BitMap * tempMap, const string & bitmapFile, const string &pieceFile, unsigned int & blockSize);
    void writeDisk(); //处理客户端的写事件,定长
    void readCache(); //处理客户端的读事件
    //bool checkRequest(); //检查客户端请求是否合理
    void openFile(); //根据hash创建文件
    void cleanData(); //清空接收缓存区，防止影响别的线程处理该连接
    bool writevClient(const unsigned int& blockSize, const unsigned int& offset, const unsigned int&  len, const int& index );

    struct iovec m_iv[3]; //成功请求的时候，将采用writev来执行操作。

};
// 入口类
class Entrance
{
public:
    Entrance();
    void parseParameters( int argc, char* const* argv ); //解析这个程序运行的参数
    void initProgram(); // 初始化 程序的必要的东西


private:
    bool openTCP_Process( shared_ptr <configureInfo> configurefile ); //服务器开启TCP服务（进程池）
    void openTCP_Thread( shared_ptr <configureInfo> configurefile );
    char* fileName; //存储配置文件名称
    //nt setNonBlocking(int sockfd);
    //queue<userInfo * > postUserInfo; //用来收集监听到的事件
    void setup_sig_pipe( int epollfd ); // 初始化信号管道
    // queue<userInfo *> sendPostData; 优化的时候再考虑这个问题

};

#endif