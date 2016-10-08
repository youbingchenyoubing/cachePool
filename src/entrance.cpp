# include "entrance.h"
//# include <stdio.h>
int CacheConn::m_epollfd = -1;
int CacheConn::m_user_count = 0;
DiskManager* diskInstance;
static ConfigureManager* configureInstance;
//static UserManager * userInstance = NULL;
static CacheManager* cacheInstance;
static shared_ptr <configureInfo> configurefile;
static rwlocker RW_Lock_count;
void CacheConn::init( int sockfd, const sockaddr_in& client_addr )
{
    //m_epollfd = epollfd;

    m_sockfd = sockfd;

    m_address = client_addr;

    m_write = nullptr;
    data = nullptr;
    /*以下两行是为了避免TIME_WAIT状态为了调试，实际应用将其去掉*/
    //int reuse =  1;
    //setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    addfd( m_epollfd, sockfd, true );
    RW_Lock_count.wrlock();
    m_user_count++;
    //int m_user_count_copy = m_user_count;
    RW_Lock_count.unlock();
    init();
#ifdef __CACHE_DEBUG
    cout << "套接字描述符是:" << m_sockfd << "监听客服端的IP是:" << inet_ntoa( m_address.sin_addr ) << "端口是:" << ntohs(m_address.sin_port) << endl;
#endif
#ifdef __LOG_DEBUG
    LOG_ERROR(__FILE__, __LINE__, "新连接:%s:%d", inet_ntoa( m_address.sin_addr ), ntohs(m_address.sin_port));
#endif
    /*
    #ifdef __LOG_DEBUG
        LOG_INFO(__FILE__, __LINE__, "连接的套接字描述符是%d,ip:%s,port:%d", m_sockfd, inet_ntoa( m_address.sin_addr ), m_address.sin_port);
    #endif
    */
}
void CacheConn::init()
{

    memset( m_buf, '\0', BUFFER_SIZE );
    //memset( m_write, '\0', W_DATA );
    if (m_write != nullptr) //一定要释放空间
    {
        delete [] m_write;
        m_write = nullptr;
    }
    if (data!= nullptr)
    {
        delete data;
        data = nullptr;
    }
    memset( m_iv, 0, sizeof( m_iv ) );
    m_read_idx = 0;
    m_write_idx = 0;

}
//主动关闭连接
void CacheConn::close_conn(int type)
{

    if ( ( m_sockfd != -1 ) )
    {

        removefd( m_epollfd, m_sockfd );
        //close(m_sockfd);
        m_sockfd = -1;
        RW_Lock_count.wrlock();
        m_user_count --;
        //int m_user_count_copy = m_user_count_copy;
        RW_Lock_count.unlock();
#ifdef __CACHE_DEBUG
        //cout << "关闭了客户端的连接" << endl;
        cout << "被关闭的用户IP是:" << inet_ntoa( m_address.sin_addr ) << "端口是:" << ntohs(m_address.sin_port) << endl;
        //cout << << endl;
#endif
#ifdef __LOG_DEBUG
        //测试使用
        LOG_INFO(__FILE__, __LINE__, "关闭了客户端的连接:%s:%d,type=%d", inet_ntoa( m_address.sin_addr ), ntohs(m_address.sin_port), type);
#endif
    }

}
/*线程入口处理函数*/
void  CacheConn::process()
{

    if (!read())
    {
#ifdef __CACHE_DEBUG
        cout << "无效协议" << m_buf << "，服务器主动关闭连接" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "无效协议:%s服务器主动关闭连接:%s:%d", m_buf, inet_ntoa( m_address.sin_addr ), ntohs(m_address.sin_port));
#endif
        close_conn(1);
        goto end;
        //goto end;
    }

    reset_oneshot(m_epollfd, m_sockfd);
end:
    init();

}

//解析用户的请求
bool CacheConn::read()
{

    int idx = 0;
    //int indxW = 0; //收集用户的Content
    int ret =  -1;
    // bool writeflag  = false;  // 标志解析协议是否正确
    while ( true )
    {
        idx = m_read_idx;
        //indxW = m_read_idx;
        ret = recv( m_sockfd, m_buf + idx, BUFFER_SIZE - 1 - idx, 0 );
        // 操作错误，关闭客户端连接，但是如果是暂时无数据可读，则退出循环
        if (ret == -1)
        {
            /*对于非阻塞的IO，下面条件表示数据 已经读取完毕，此后，epoll就能再次触发sockfd上的EPOLLIN事件，以驱动下一次操作*/
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                if (idx == 0)
                {
                    ResponesClient(ERRORDATA, NULL);
                    return  true;
                } else
                {
                    continue;
                }
                //continue;
            }
            return false;
        }
        else if (ret == 0)
        {
            //cout << "hello world" << endl;
            if (idx == 0)
                break;

            continue;
        }

        // 客户端关闭连接
        else
        {
            m_read_idx += ret;
            /*
            #ifdef __CACHE_DEBUG
                        cout << "user send content is " << m_buf << endl;
            #endif
            */
            for ( ; idx < m_read_idx; ++idx )
            {
                if ( ( idx >= 1 ) && ( m_buf[idx - 1] == '\r' ) && ( m_buf[idx] == '\n' ) )
                {
                    break;
                }
            }
            if ( idx == m_read_idx )
            {
                continue;
            }
            m_buf[idx - 1] = '\0';
            if ( ParseRequest() ) //解析报文
            {
                if (  strcmp(clientData.type , "read") == 0 )
                {
#ifdef __CACHE_DEBUG
                    cout << "客户是发送过来读事件" << endl;
#endif
#ifdef __LOG_DEBUG
                    LOG_INFO(__FILE__, __LINE__, "客户是发送过来读事件");
#endif
                    if ( clientData.len <= 0 )
                    {
#ifdef __CACHE_DEBUG
                        cout << "客户读取的数据小于0,拒绝" << endl;
#endif
#ifdef __LOG_DEBUG
                        LOG_ERROR(__FILE__, __LINE__, "客户读取的数据小于0,拒绝");
#endif
                        return false;
                    }
                    //modfd( m_epollfd, m_sockfd, EPOLLOUT ); /*为了防止上一个请求还没处理，客户端发来另外一个请求*/
                    readCache();
                    return  true;
                }
                else if ( strcmp(clientData.type , "write") == 0 )
                {
                    //removefd(m_epollfd,m_sockfd);
#ifdef __CACHE_DEBUG
                    cout << "客户端发送过来的是写事件" << endl;
#endif
#ifdef __LOG_DEBUG
                    LOG_INFO(__FILE__, __LINE__, "客户端发送过来的是写事件");
#endif
                    if ( clientData.len <= 0 )
                    {
#ifdef __CACHE_DEBUG
                        cout << "客户发送读数据小于0,拒绝" << endl;
#endif
#ifdef __LOG_DEBUG
                        LOG_WARNING(__FILE__, __LINE__, "客户发送读数据小于0,拒绝");
#endif

                        return false;
                    }
                    /*检查磁盘数据是否与服务器对齐*/
                    if ( ( clientData.offset ) % (configurefile->blockSize) )
                    {
                        ResponesClient( ERRORALIGN, NULL );
#ifdef __CACHE_DEBUG
                        cout << "ERROR!!!对齐错误" << endl;
#endif
#ifdef __LOG_DEBUG
                        LOG_ERROR(__FILE__, __LINE__, "ERROR!!!对齐错误,%u-%u", clientData.offset, configurefile->blockSize);
#endif
                        return false;

                    }
                    int i = 0;
                    if (m_write == nullptr)
                        m_write = new char[configurefile->blockSize];
                    if (m_write == nullptr)
                    {
#ifdef __LOG_DEBUG
                        LOG_ERROR(__FILE__, __LINE__, "创建接收缓冲空间失败");
#endif
                    }
                    for ( ++idx; idx < m_read_idx; ++idx, ++i )
                    {
                        m_write[i]  = m_buf[idx];
                    }

                    m_write_idx = i; //防止把写命令中的数据读出来
                    writeDisk();
                    return true;


                }// end else
                else if (strcmp(clientData.type , "open") == 0)
                {
#ifdef __CACHE_DEBUG
                    cout << "客户端要创建文件" << endl;
#endif
                    if (clientData.offset == 0)
                    {
#ifdef __CACHE_DEBUG
                        cout << "创建文件的长度小于0,拒绝" << endl;
#endif
#ifdef __LOG_DEBUG
                        LOG_WARNING(__FILE__, __LINE__, "创建文件的长度小于0,拒绝");
#endif
                        return false;
                    }
                    //modfd( m_epollfd, m_sockfd, EPOLLOUT );
                    openFile();
                    return true;

                } //else if (clientData.type == "open")
            }//end  if(ParseRequest())

            else
            {
#ifdef __CACHE_DEBUG
                cout << "客户端了发送错误数据" << endl;
#endif
#ifdef __LOG_DEBUG
                LOG_ERROR(__FILE__, __LINE__, "客户端了发送错误数据%s", m_buf);
#endif
                return false;
            }
        }//end else
    }// end while(true)

    return false;
}
BitMap *  CacheConn::rebuildBitMap(BitMap * tempMap, const string & bitmapFile, const string &pieceFile, unsigned int & blockSize)
{
    //const char * bit = bitmapFile.c_str();
    if (tempMap != nullptr)
    {
        if (tempMap->isChanged)
        {
            tempMap->restoreBitMap();
            delete tempMap; //释放空间
            tempMap = nullptr;
        }
    }
    unsigned int fileSize = diskInstance->getFileSize(pieceFile.c_str());
    unsigned int bitmapSize = (fileSize % blockSize == 0 ? fileSize / blockSize : (fileSize / blockSize + 1));
    tempMap = new BitMap(bitmapFile, bitmapSize);
    return tempMap;
}
//改进兼容文件大小是否已知
void CacheConn::writeDisk()
{
    unsigned int idx = 0;
    int ret = 0;
    unsigned int  W_DATA = configurefile->blockSize;
    /*对每次用户发过来的数据不确定，将
    长度切分成服务器接收buffer的几倍*/
    int times = ( clientData.len ) / W_DATA;

    if ( times * W_DATA < clientData.len ) /*处理不能整除的问题*/
    {
        times += 1;
    }
    string file = "";
    file += configurefile->filePath;
    string hashFile = clientData.fileName;
    if ( !parseHash( configurefile->levels, file, hashFile ) )
    {
        cleanData();
        ResponesClient( HASHERROR, NULL ); //hash文件解析错误
        return;
    }
    unsigned int code = 0;
    //unsigned int fileSize = 0;

    unsigned int  writeLen = clientData.len;
    unsigned int fileIndex;

    unsigned int startBlock = ( clientData.offset ) / W_DATA;
    string bitmapFile;
    string pieceFile;
    unsigned int offset;
    unsigned int searchIndex;
    BitMap   *tempMap = nullptr;
    setblocking(m_sockfd);
    for ( int i = 0; i < times; ++i )
    {

        while ( true )
        {
            if ( i != times - 1 && m_write_idx == W_DATA )
            {
                //cout<<"写中间数据"<<endl;
                break;
            }
            else if ( i == times - 1 && ( ( clientData.len ) - i * W_DATA ) == m_write_idx )
            {
                //cout<<"写最后一块数据"<<endl;
                break;
            }
            idx = m_write_idx;
            ret = recv( m_sockfd, m_write + idx, W_DATA - idx, 0 );
            if ( ret <= 0 )
            {
                setnonblocking(m_sockfd);
                close_conn(2);
                goto end1;
            }
            else
            {
                m_write_idx += ret;
            }
        }//end while(true)xml
        pieceFile = file;
        offset =  ( clientData.offset ) + i * W_DATA;
        fileIndex = diskInstance->getRealFile(pieceFile, clientData.offset);
        bitmapFile = pieceFile + ".bitmap";
        if (tempMap == nullptr || tempMap->bitFile != bitmapFile)
        {
            tempMap = rebuildBitMap(tempMap, bitmapFile, pieceFile, W_DATA);
            if (tempMap == nullptr)
            {
#ifdef __LOG_DEBUG
                LOG_ERROR(__FILE__, __LINE__, "简历位图信息失败");
#endif
            }
        }
        searchIndex = (startBlock + i) - fileIndex * (diskInstance->times);
        int temp = tempMap->getByteValue(searchIndex);
        if ( temp == 0 )
        {
            //更新内存缓存区
            cacheInstance->updateCache(clientData.fileName, m_write, startBlock + i, m_write_idx);
            if ( !( diskInstance->writeOneBuff2(pieceFile, m_write, ( ( clientData.offset ) + i * W_DATA ), m_write_idx ) ) )
            {
                code = ERRORWRITE;
#ifdef __CACHE_DEBUG
                cout << "在写第" << i + 1 << "块出错了" << endl;
#endif
#ifdef __LOG_DEBUG
                LOG_ERROR(__FILE__, __LINE__, "在写第%d块出错了", i + 1);
#endif

                goto  end;
            }

            //改变位图
            tempMap->setByteValue(searchIndex, 1 );
            //bitMapChanged = true;
        }
        else if ( temp == -1 )
        {

            code = BADBITMAP;
            goto end;
        }
        writeLen -= m_write_idx;

        m_write_idx = 0;
        memset( m_write, '\0', W_DATA );

    }//end for
    if (writeLen == 0)
        code = OKW;
    else {
        code = ERRORW;
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!写长度不一致" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "写长度不一致,还有%u数据没写成功", clientData.len);
#endif
    }
end:
    setnonblocking(m_sockfd);
end1:
    cleanData();
    if (tempMap != nullptr) //位图信息变化,更新到磁盘
    {
        if (tempMap->isChanged)
        {
            tempMap->restoreBitMap();
        }
        delete tempMap;
        tempMap = nullptr;
    }
    ResponesClient(code, NULL);

}

// 如果写文件出错，也要把接收缓冲区的东西丢弃,因为epoll采用的是ET模式
void CacheConn::cleanData()
{
    int ret;
    unsigned int idx = 0;
    while (true)
    {
        idx = m_write_idx;
        ret = recv( m_sockfd, m_buf + idx, BUFFER_SIZE - idx, 0 );
        if (ret <= 0)
        {

            return;
        }

        else {
            m_write_idx += ret;
            if (m_write_idx >= BUFFER_SIZE - 1)
            {
                m_write_idx = 0;
                memset( m_buf, '\0', BUFFER_SIZE );
            }
        }
    }

}
// 处理客户请求创建文件
void CacheConn::openFile()
{
    string file = configurefile->filePath;
    string hashFile = clientData.fileName;
    if (!parseHash(configurefile->levels, file, hashFile))
    {
        cleanData();
        ResponesClient( HASHERROR, NULL );
        return;
    }
    if (diskInstance->openNewFile(file, clientData.offset))
    {
        cleanData();
        ResponesClient(OPENOK, NULL);

    }
    else {
        cleanData();
        ResponesClient(OPENERROR, NULL);

    }

    //modfd( m_epollfd, m_sockfd, EPOLLIN );
}
//处理客户端读的请求
void CacheConn::readCache()
{

    unsigned int blockSize = configurefile->blockSize;
    unsigned int beginBlock =  ( clientData.offset ) / blockSize;
    unsigned int endBlock = ( clientData.offset + clientData.len - 1 ) / blockSize;
    /*下面这句话应该去掉*/
    /*
    if ( ( endBlock - beginBlock + 1 )*blockSize < ( clientData.len ) )
    {
        endBlock += 1;
    }*/
#ifdef __CACHE_DEBUG
    cout << "beginBlock = " << beginBlock << ",endBlock=" << endBlock << ", ";
#endif
#ifdef __LOG_DEBUG
    LOG_INFO(__FILE__, __LINE__, "客户端请求从%u块%s%u%s", beginBlock, "到", endBlock, "块");
#endif
    if (data == nullptr)
        data = new LinkListNode(blockSize);
    if (data == nullptr)
    {

        ResponesClient(ERRORSPACE, NULL);
        return;
    }
    unsigned int g_Info = 0;
    unsigned  int i = beginBlock;
    for (; i <= endBlock; ++i )
    {
        cacheInstance->searchBlock( clientData.fileName, i, g_Info, data);

        if ( data->length == 0)
        {

            ResponesClient( g_Info, NULL );

            return;

        }

        if (!writevClient(blockSize, clientData.offset, clientData.len, i ))
        {
            break;
        }
    }
//高并发测试
#ifdef __LOG_DEBUG
    if (i == endBlock)
        LOG_ERROR(__FILE__, __LINE__, "成功处理客户端的请求:%s:%d,总共阅读了%u", inet_ntoa( m_address.sin_addr ), ntohs(m_address.sin_port), (endBlock - beginBlock + 1)*blockSize);
#endif
}
bool CacheConn::writevClient(const unsigned int& blockSize, const unsigned int& offset, const unsigned int&  len, const int& index )
{
    size_t begin_offset = 0;
    size_t end_offset = 0;
    if ( offset > index * blockSize )
    {
        begin_offset = offset - index * blockSize;
    }
    if ( ( offset + len ) <= ( ( index + 1 )*blockSize - 1 )) //漏了个等号
    {
        end_offset = ( ( index + 1 ) * blockSize - 1 ) - offset - len + 1;
    }
    size_t send_count = blockSize - begin_offset - end_offset;

    if ( (data->length - begin_offset) < send_count )
    {
        // 这个 只是简单处理客户端请求越界，这个可以以后完善
        ResponesClient( NOBLOCK, NULL );
        //modfd( m_epollfd, m_sockfd, EPOLLIN );
        return false;
    }
    if ( send_count <= 0 || send_count > blockSize )
    {
#ifdef __CACHE_DEBUG
        cout << "头部协议出错" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "头部协议出错");
#endif
        return false;
    }


    char headInfo[128];
    //char ivInfo[200];

    memset( headInfo, '\0', 128 );
    //memset( ivInfo, '\0', 200 );
    char finish[3]  = "\r\n";
    int ret = snprintf( headInfo, 128, "%d/%zd/%zd\r\n", CACHEMISS, index * blockSize + begin_offset, send_count + strlen(finish) );
    if ( ret == -1)
    {
#ifdef __CACHE_DEBUG
        cout << "头部协议过长" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "头部协议过长");
#endif
        return false;
    }
#ifdef __LOG_DEBUG
    LOG_INFO(__FILE__, __LINE__, "发送给客户端:%s:%d,的头部是:%s", inet_ntoa( m_address.sin_addr ), ntohs(m_address.sin_port), headInfo);
#endif
    void * newData =  data->data;

    m_iv[0].iov_base = headInfo;
    m_iv[0].iov_len  = strlen( headInfo );

    m_iv[1].iov_base = newData + begin_offset;
    m_iv[1].iov_len = send_count;
    m_iv[2].iov_base = finish;
    m_iv[2].iov_len = strlen(finish);

    setblocking(m_sockfd);

    ret = writev(m_sockfd, m_iv, 3);

    if (ret <= 0)
    {
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "发送读数据失败,错误代码为:%d,关闭了客户端的连接:%s:%d", errno, inet_ntoa( m_address.sin_addr ), ntohs(m_address.sin_port));
#endif
        setnonblocking(m_sockfd);
        close_conn(3);
        return false;
    }
    if (ret == (send_count + strlen( headInfo ) + strlen(finish)))
    {
#ifdef __CACHE_DEBUG
        cout << "data=" << ( data ) << "里面的偏移量是" << begin_offset << endl;

#endif
#ifdef __LOG_DEBUG
        //LOG_ERROR(__FILE__,__LINE__,"头部是:%s",headInfo);
        LOG_INFO(__FILE__, __LINE__, "发送第%d块给客户端", index);
#endif
        setnonblocking(m_sockfd);
        return true;

    }
    else
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!发送数据不一致,本应该发送" << ( strlen( headInfo ) + send_count ) << "字节,却只发送了:" << ret << "字节" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "ERROR!!!发送数据不一致,本应该发送%d字节,却只发送了:%d,客户端:%s:%d", ( strlen( headInfo ) + send_count ), ret, inet_ntoa( m_address.sin_addr ), ntohs(m_address.sin_port));
#endif
        setnonblocking(m_sockfd);
        close_conn(3);
        return false;
    }

}

//解析应用层的头部协议
bool CacheConn::ParseRequest()
{
    //cout<<"解析头部"<<endl;
    char* buf = m_buf;
    int i = 0;
    bool isbegin = true;
    while ( *buf != '\0' )
    {

        if ( *buf == '/' )
        {
            *buf = '\0';
            isbegin = true;
            ++i;
        }
        else if ( isbegin )
        {
            switch ( i )
            {
            case 0:
                clientData.type = buf;
                break;
            case 1:
                clientData.fileName = buf;
                break;
            case 2:
                clientData.offset = strtoul( buf, NULL, 10 );
                break;
            case 3:
                clientData.len = strtoul( buf, NULL, 10 );
                break;
            //case 4: clientData->flag = buf;break;
            default:
                break;
            }
            isbegin = false;
        }//end else if
        buf++;
    }//end while
    if (  strcmp(clientData.type, "read") == 0 || strcmp(clientData.type, "write") == 0 || strcmp(clientData.type, "open") == 0 )
    {

        return true;
    }

    else
    {
#ifdef __CACHE_DEBUG
        cout << "客户端命令:" << clientData.type << "不合法" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "客户端命令:%s不合法", m_buf);
#endif
        return false;
    }

}

//响应客户的请求 (没有发送成功数据的)
void CacheConn::ResponesClient( unsigned int code, void* data )
{
    if ( m_sockfd != -1 )
    {

        //char codeTrans[5];
        //cout<<code<<endl;
        char  sendData[10];
        memset( sendData, '\0', 10 );
        int ret = snprintf( sendData, sizeof( sendData ), "%u\r\n", code );
        if (ret == -1)
        {
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "sprintf函数运行出错%d", errno);
#endif
        }
        //strcat( sendData, codeTrans );
        //strcat( sendData, "\r\n" );
        //int ret;
        /*为了数据完全发送设为阻塞*/
        setblocking(m_sockfd);
        ret = send( m_sockfd, sendData, strlen( sendData ), 0 );
        if (ret <= 0)
        {
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "发送响应客户端:%s:%d,错误代码为:%d,", errno, inet_ntoa( m_address.sin_addr ), ntohs(m_address.sin_port));
#endif
            close_conn(4);
        }
        else if (ret == strlen(sendData))
        {
#ifdef __CACHE_DEBUG
            cout << "发送响应成功" << endl;
#endif
#ifdef __LOG_DEBUG
            LOG_INFO(__FILE__, __LINE__, "发送响应成功");
#endif
        }
        else {
#ifdef __CACHE_DEBUG
            cout << "发送不完全" << endl;
#endif
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "发送信息不完全,少发了%d,客户端:%s:%d", strlen(sendData) - ret, inet_ntoa( m_address.sin_addr ), ntohs(m_address.sin_port));
#endif
            close_conn(4);
        }
        setnonblocking(m_sockfd);

    }

}

Entrance::Entrance()
{
    fileName = "config.xml";

    configureInstance = CacheFactory::createConfigureManager();

}
// 解析参数
void Entrance::parseParameters( int argc, char* const* argv )
{

#ifdef __CACHE_DEBUG

    cout << "开始解析程序命令" << endl;
#endif
    char* parse;


    int i;

    for ( i = 1; i < argc ; ++i )
    {
        parse = argv[i];

        if ( *parse++ != '-' )
        {
#ifdef __CACHE_DEBUG
            std::cout << "invaild option:" << argv[i] << "\n";
#endif
            exit( 1 );
        }


        while ( *parse )
        {
            switch ( *parse++ )
            {
            case  'c':
                if ( argv[++i] )
                {
                    fileName = argv[i];

                    break;
                }
#ifdef __CACHE_DEBUG
                std::cout << "invaild parse,when parse \"c\"\n";
#endif
                return;
            } // end switch
        }// end while
    } // end for
}

//初始化这个程序
void Entrance::initProgram()
{
#ifdef __CACHE_DEBUG
    cout << "开始初始化整个程序" << endl;
#endif
    configurefile = configureInstance->readConfigure();
    //设置日志大小
    SET_LOG_SIZE(10 * 1024 * 1024);

    //设置占用磁盘空间大小
    SET_LOG_SPACE(100 * 1024 * 1024);

    parsexml::parseXML( fileName, configurefile );
#ifdef __CACHE_DEBUG
    cout << "配置信息如下" << endl;
#endif
    configureInstance->printfConfigure();
    diskInstance = DiskManager::getInstance( configurefile->diskSize, configurefile->blockSize); //将磁盘文件分块存储
    cacheInstance =  CacheFactory::createCacheManager( configureInstance );

    openTCP_Thread( configurefile );

}
void Entrance::setup_sig_pipe( int epollfd )
{
    /*忽略SIGPIPE信号*/
    int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, sig_pipefd );
    assert( ret != -1 );
    setnonblocking( sig_pipefd[1] );

    addfd( epollfd, sig_pipefd[0], false );

    addsig( SIGCHLD, sig_handler );
    addsig( SIGTERM, sig_handler );

    addsig( SIGINT, sig_handler );

    addsig( SIGPIPE, SIG_IGN );
}
//初始化线程池
void Entrance::openTCP_Thread( shared_ptr <configureInfo> configurefile )
{
    /*创建线程池*/

    ThreadManager <CacheConn>* pool = ThreadManager <CacheConn>::getInstance( configurefile->worker, MAX_EVENT_NUMBER );

    if ( !pool )
    {
        return;
    }
    /*为客户连接分配一个CaConn*/
    CacheConn* users = new CacheConn[MAX_FD];
    assert( users );

    //int user_count = 0;

    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );

    assert( listenfd >= 0 );
    struct linger tmp = {1, 0};
    setsockopt( listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof( tmp ) );
    int ret = 0;

    struct sockaddr_in address;

    bzero( &address, sizeof( address ) );

    address.sin_family = AF_INET;

    //char * ip = configurefile->ip;

    assert( ( configurefile->ip ).c_str() != NULL );

    inet_pton( AF_INET, ( configurefile->ip ).c_str(), &address.sin_addr );

    address.sin_port = htons ( configurefile->port );

    ret = bind( listenfd, ( struct  sockaddr* )&address, sizeof( address ) );

    assert( ret != -1 );

    ret = listen( listenfd, 511 ); //这是一个经验值
    epoll_event events[MAX_EVENT_NUMBER];

    int epollfd = epoll_create(65535 );

    assert( epollfd != -1 );

    addfd( epollfd, listenfd, false );

    CacheConn::m_epollfd = epollfd;
    setup_sig_pipe( epollfd ); //初始化信号

    while ( true )
    {
        int number = epoll_wait(  epollfd, events, MAX_EVENT_NUMBER, -1 );

        if ( ( number < 0 ) && ( errno != EINTR ) )
        {
#ifdef __CACHE_DEBUG
            cout << "epoll failure\n";
#endif
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "epoll wait出错");
#endif
            break;
        }
        for ( int i = 0; i < number; ++i )
        {
            int sockfd = events[i].data.fd;

            if ( sockfd == listenfd )
            {
                struct sockaddr_in client_address;

                socklen_t client_addrlength = sizeof( client_address );

                int connfd;
                //防止accept的问题
                while ((connfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength )) > 0)
                {
                    //RW_Lock_count.rdlock();
                    if ( CacheConn::m_user_count >= MAX_FD )
                    {

#ifdef __CACHE_DEBUG
                        cout << "WARNING!!! server is overload " << endl;
#endif
#ifdef __LOG_DEBUG
                        LOG_ERROR(__FILE__, __LINE__, "服务器负载过重，拒绝了连接:%s:%d", inet_ntoa( client_address.sin_addr ), client_address.sin_port);
#endif
                        char* info = "500\r\n";
                        send( connfd, info, strlen( info ), 0 );
                        continue;
                    }
                    //RW_Lock_count.unlock();
                    users[connfd].init( connfd, client_address );
                }
                if (connfd == -1) {
                    if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
                        perror("accept");
                }

            } //end if(sockfd == listenfd)
            //信号处理
            else if ( ( sockfd == sig_pipefd[0] ) && ( events[i].events & EPOLLIN ) )
            {
                //int sig;
                char signals[1024];

                ret  = recv( sig_pipefd[0], signals, sizeof( signals ), 0 );
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
                        case SIGTERM:
                        case SIGHUP:
                        case SIGINT:
                        {
#ifdef __CACHE_DEBUG
                            cout << "程序被kill掉" << endl;
#endif
#ifdef __LOG_DEBUG
                            LOG_INFO(__FILE__, __LINE__, "程序被kill掉");
#endif
                            cacheInstance->deleteAll();

                            if ( users )
                            {
                                delete []  users;
                                users = nullptr;

                            }

                            if ( pool )
                            {
                                delete pool;
                                pool = nullptr;
                                //cout << "pool 空间被释放" << endl;
                            }

                            close( epollfd );
                            close( listenfd );
#ifdef  __CACHE_DEBUG
                            cout << "程序安全退出" << endl;
#endif
#ifdef __LOG_DEBUG
                            LOG_INFO(__FILE__, __LINE__, "程序安全退出");
#endif
                            return;
                        }
                        } // end switch
                    } //  end  for

                } //end else
            }//end   else if((sockfd == sig_pipefd[0])&&(events[i].events & EPOLLIN))
            else if ( events[i].events & (EPOLLRDHUP| EPOLLHUP | EPOLLERR ) )
            {
#ifdef __LOG_DEBUG
                LOG_INFO(__FILE__, __LINE__, "捕捉EPOLL事件,关闭了客户端的连接");
#endif

                users[sockfd].close_conn(5);
            } // end  else if
            else if ( events[i].events & EPOLLIN )
            {

                if (!(pool->append( users + sockfd )))
                {
#ifdef  __CACHE_DEBUG

                    cout << "队列事件太多,无法加入队列" << endl;
#endif
#ifdef __LOG_DEBUG
                    LOG_ERROR(__FILE__, __LINE__, "队列事件太多,无法加入队列,sockfd:%d", sockfd);
#endif
                    users[sockfd].close_conn(6);
                }

            }  //end else if(events[i].events & EPOLLIN)
            else
            {
#ifdef  __CACHE_DEBUG
                cout << "未明原因" << endl;
#endif
#ifdef __LOG_DEBUG
                LOG_ERROR(__FILE__, __LINE__, "未明原因");
#endif

                users[sockfd].close_conn(7);
            }
        }// end for (int i = 0;i<number;++i)
    }// end  while(true)

    close( epollfd );
    close( listenfd );
    cacheInstance->deleteAll();
    if ( users )
    {
        delete [] users;
        users = nullptr;
    }

    if ( pool )
    {
        delete pool;
        pool = nullptr;
    }

#ifdef  __CACHE_DEBUG
    cout << "程序安全退出" << endl;
#endif
#ifdef __LOG_DEBUG
    LOG_INFO(__FILE__, __LINE__, "程序安全退出");
#endif
    return ;

}

