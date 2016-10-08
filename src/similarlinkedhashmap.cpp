#include "similarlinkedhashmap.h"

namespace cachemanager
{

/* Cache工厂类*/
// 创建cache管理实例
CacheManager* CacheFactory::createCacheManager( ConfigureManager* configureInstance )
{

    return CacheManager::getInstance( configureInstance );
}

// 创建配置文件管理实例
ConfigureManager*   CacheFactory::createConfigureManager()
{

    return ConfigureManager::getInstance();
}


IOManager* CacheFactory::createIOManager()
{

    return  IOManager::getInstance();
}


IOManager* CacheManager::ioInstance = CacheFactory::createIOManager();

CacheManager* CacheManager:: getInstance( ConfigureManager* configureInstance )
{

    static CacheManager cacheInstance( configureInstance );

    return &cacheInstance;
}

CacheManager::CacheManager( ConfigureManager* configureInstance )
{

    mapManager.clear();
    linkListHead = linkListTail = nullptr;
    //ioInstance = CacheFactory::createIOManager();
    configureContent = configureInstance-> readConfigure();
    // cout<<"configureContent->blockSize="<<configureContent->blockSize;
    guardNode = new  LinkListNode( configureContent->blockSize );

    if ( !guardNode )
    {

        throw std::exception();

    }


}
/*
CacheManager::~CacheManager()
{
  ~Manager();
}*/
void CacheManager::updateCache(const char *fileName, const char * newData, const unsigned int & index, const unsigned int & length)
{
    string file = fileName;
    m_maplocker.wrlock();
    if (mapManager.find(file) != mapManager.end() && mapManager[file].find(index) != mapManager[file].end())
    {   //更新cache
        //m_maplocker.unlock();
        //m_maplocker.wrlock();
        auto oldLinkList = (mapManager[file][index]);
        memcpy(oldLinkList->data, (const void *)newData, length);
        /*注意缓存大小*/
        oldLinkList->length = length > oldLinkList->length ? length : oldLinkList->length;
        linkListInsertHead(oldLinkList);
#ifdef  __CACHE_DEBUG
        cout << "更新cache成功" << endl;
#endif

    }
    m_maplocker.unlock();

}
void  CacheManager::searchBlock( const char* fileName, unsigned int & index, unsigned int& g_Info, LinkListNode *data )
{

#ifdef __CACHE_DEBUG
    cout << "index=" << index << endl;
#endif
    string file = fileName;
    m_maplocker.rdlock();
    map<string, map<unsigned int, LinkListNode*> > ::iterator it = mapManager.find( file );
    if ( it == mapManager.end() )
    {
        if ( curBlockNum < configureContent->maxBlockNum )

        {
            m_maplocker.unlock();
            getIoBlock( fileName, index, g_Info, data );
        }
        else
        {
            m_maplocker.unlock();
            getIoBlock2( fileName, index, g_Info, data );
        }
    }
    else
    {

        map<unsigned int, LinkListNode*>::iterator it_index = it->second.find( index );

        if ( it_index == it->second.end() )
        {


            if ( curBlockNum < configureContent->maxBlockNum )
            {
                m_maplocker.unlock();
                getIoBlock( fileName, index, g_Info, data);
            }

            else
            {
                m_maplocker.unlock();
                getIoBlock2( fileName, index, g_Info, data);
            }

        } // end  if(it_index == it->second.end())
        else  //命中cache
        {
#ifdef __CACHE_DEBUG
            cout << "cache 命中了" << endl;
            cout << "命中数据是" << it_index->second->data << endl;
#endif
#ifdef __LOG_DEBUG

            LOG_INFO(__FILE__, __LINE__, "cache 命中了,命中数据是:%u", it_index->second->data);
#endif
            copyData(data, it_index->second);
            m_maplocker.unlock();
            m_maplocker.wrlock();
            //为 了 防止其他线程获得锁已经修改这个指向
            if (mapManager.find(file) != mapManager.end() && mapManager[file].find(index) != mapManager[file].end())
            {
                if ( linkListDisplaceNode( mapManager[file][index] )) {
#ifdef __CACHE_DEBUG
                    cout << "运行LRU 算法成功" << endl;
#endif
#ifdef __LOG_DEBUG
                    LOG_INFO(__FILE__, __LINE__, "运行LRU 算法成功");
#endif
                }
                else {
#ifdef __LOG_DEBUG
                    LOG_ERROR(__FILE__, __LINE__, "运行LRU 算法失败");
#endif
                }
            }
            m_maplocker.unlock();
            return ;

        }
    } //end else{}

}
void  CacheManager::copyData(LinkListNode *data, LinkListNode *copy)
{
    if (copy != nullptr && data != nullptr)
    {

        // data->name = copy->name;
        data->offset = copy->offset;
        data->length = copy->length;
        memcpy( data->data, ( const void* )( copy->data ), copy->length );

    }
}
//不满
void CacheManager::getIoBlock( const char* fileName, unsigned int &index, unsigned int& g_Info, LinkListNode *data )
{

    string file = fileName;

    LinkListNode* temp = new LinkListNode( file, index, configureContent->blockSize );
    if (!temp)
    {
        data->length = 0;
        return;
    }

    if ( !( ioInstance->AIORead2( fileName, index, configureContent, g_Info, temp, false ) ) )
    {
        //mutex_Lock.unlock();

        if ( temp != nullptr)
        {
            delete temp;
            temp = nullptr;
        }
        //curBlockNum--;
        data->length = 0;
        return;
        //return nullptr;
    }

    m_maplocker.wrlock();
    if ( mapManager.find( file ) != mapManager.end() && mapManager[file].find( index ) != mapManager[file].end() )
    {
        copyData(data, mapManager[file][index]);
        m_maplocker.unlock();
        //m_maplocker.unlock();
        if ( temp != nullptr)
        {
            delete temp;
            temp = nullptr;
        }
#ifdef __CACHE_DEBUG
        cout << "假命中" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_INFO(__FILE__, __LINE__, "假命中");
#endif
        return;

    }
    //m_maplocker.unlock();
    //m_maplocker.wrlock();
    linkListInsertHead( temp );
    addMap( file, index );
    if ( curBlockNum < configureContent->maxBlockNum )
    {
        curBlockNum++;
    }
    else
    {
        deleteLinklistTail();
    }
    //mutex_Lock.unlock();
    copyData(data, linkListHead);
    m_maplocker.unlock();
    //return linkListHead;
    //m_maplocker.unlock();


}
void CacheManager::getIoBlock2( const char* fileName, unsigned int &index, unsigned int& g_Info, LinkListNode *data)
{

#ifdef __CACHE_DEBUG
    cout << "缓存空间已满,现在是LRU算法" << endl;
#endif
#ifdef __LOG_DEBUG
    LOG_INFO(__FILE__, __LINE__, "缓存空间已满,现在是LRU算法");
#endif
    string file = fileName;

    if ( !( ioInstance->AIORead2( fileName, index, configureContent, g_Info, guardNode, true ) ) )
    {
        //mutex_Lock.unlock();
        //copyData(data, nullptr);
        data->length = 0;
        return ;
    }

    m_maplocker.wrlock();
    if ( mapManager.find( file ) != mapManager.end() && mapManager[file].find( index ) != mapManager[file].end() )
    {
        mutex_Lock.unlock(); //一定要释放
        copyData(data, mapManager[file][index]);
        m_maplocker.unlock();
#ifdef __CACHE_DEBUG
        cout << "假命中" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_INFO(__FILE__, __LINE__, "假命中");
#endif
        return;
    }
    //m_maplocker.unlock();
    //m_maplocker.wrlock();
    deleteMap();
    addMap( file, index );
    copyData(data, linkListHead);
    m_maplocker.unlock();
    //return linkListHead;

}
bool CacheManager::deleteMap()
{
    linkListInsertHead( guardNode );


    guardNode = linkListTail;

    linkListTail = guardNode->pre;
    string name = guardNode->name;
    unsigned int index = guardNode->offset;
    guardNode->pre = nullptr;
    guardNode->post = nullptr;
    mutex_Lock.unlock();
    linkListTail->post = nullptr;



    /*在map中删除guardNode中fileName和index*/
    deleteMap2( name, index );
    return true;

}
bool CacheManager::deleteMap2( const  string& fileName, unsigned int &index )
{
    /*在map中删除LinkListTail中fileName和index*/
    mapManager[fileName][index] = nullptr;
    map<unsigned int, LinkListNode*>* mapSecond = &( mapManager[fileName] );
    mapSecond->erase( index );

    if ( mapSecond->empty() )
    {
        mapManager.erase( fileName );
    }
    return true;
}
bool CacheManager::addMap( string const& fileName, unsigned int &index )
{

    map<string, map<unsigned int, LinkListNode*> > ::iterator it = mapManager.find( fileName );

    if ( it == mapManager.end() )
    {
        map<unsigned int, LinkListNode*> temp;
        temp[index] = linkListHead;

        mapManager[fileName] = temp;

    }
    else
    {

        ( it->second )[index] = linkListHead;


    }
    return true;

}
bool CacheManager::linkListInsertHead( LinkListNode* nodeInfo )
{
    if ( nodeInfo == nullptr )
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR: 链表插入时候，传进来竟然是空节点，不能忍" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "链表插入时候，传进来竟然是空节点，不能忍");
#endif
        return false;
    }
    if ( linkListHead == nullptr && linkListTail == nullptr ) //链表是为 空的
    {
        linkListHead = linkListTail = nodeInfo;

        linkListHead->pre = nullptr;

        linkListTail->post = nullptr;

    }

    else
    {
        if ( linkListHead == nullptr || linkListTail == nullptr ) // 正常情况这种情况是永远不会成立的，但是为了防止出现不可控的情况
        {
#ifdef __CACHE_DEBUG
            cout << "EORROR: 在类CacheManager中的函数linklistInsertHead,链表头指针 和 尾部 指针 中有一个指向空指针，这个是明显有问题,请注意" << endl;
#endif
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "类CacheManager中的函数linklistInsertHead,链表头指针 和 尾部 指针 中有一个指向空指针，这个是明显有问题");
#endif
            return false;
        }

        else
        {
            linkListHead-> pre = nodeInfo;
            nodeInfo->post = linkListHead;
            nodeInfo->pre = nullptr;
            linkListHead = nodeInfo;
        }


    }
    return true;
}

bool CacheManager::linkListDisplaceNode( LinkListNode* nodeInfo )
{
    if ( nodeInfo == nullptr || linkListHead == nullptr || linkListTail == nullptr )
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR: 链表替换时候，传进来竟然是空节点，不能忍" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "链表替换时候，传进来竟然是空节点，不能忍");
#endif
        return false;
    }
    if ( linkListHead == nodeInfo ) // 命中的节点就是首部节点，就不要替换了
    {
        return true;
    }

    else if (  nodeInfo == linkListTail ) //这个运气也挺好嘛，尾部的数据被命中了。
    {
        linkListTail = nodeInfo->pre; // 改变 尾部指针的位置
        linkListTail-> post = nullptr;
        nodeInfo->post = linkListHead; //将原来首部节点 放在nodeInfo后面

        linkListHead-> pre = nodeInfo;

        nodeInfo-> pre = nullptr;

        linkListHead = nodeInfo;

    }

    else  //要替换节点 处于一般情况
    {


        nodeInfo->pre->post = nodeInfo->post;

        nodeInfo->post->pre = nodeInfo->pre;

        // 这时断开nodeinfo原有的 连接

        nodeInfo->post = linkListHead;

        linkListHead -> pre = nodeInfo;

        nodeInfo->pre = nullptr;

        linkListHead  = nodeInfo;

    }

    return  true;
}
bool CacheManager::deleteLinklistTail()
{
    LinkListNode* temp = linkListTail;
    if ( linkListTail != nullptr && linkListTail != linkListHead )
    {
        linkListTail = linkListTail->pre;
        linkListTail->post = nullptr;
        deleteMap2( temp->name, temp->offset );
        if (temp != nullptr)
        {   delete temp;
            temp = nullptr;
        }
        return true;
    }
    else
    {

#ifdef __CACHE_DEBUG
        cout << "ERROR: 只剩下一个链表，明显出错 ，除非cache只有一个块" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "只剩下一个链表，明显出错 ，除非cache只有一个块");
#endif
        return false;
    }

}
// 结束cache管理类实现
// 开始实现配置 文件类的实现
/*
ConfigureManager::~ConfigureManager()
{
  ~Manager();
}*/
ConfigureManager::ConfigureManager(): configureFile( new configureInfo )
{
    //configureFile = static_cast<configureInfo *>(ngx_palloc(memory_Instance->getClassPool(),sizeof(configureFile)));
    if ( configureFile )
    {
        //cout<<"test"<<endl;
        configureFile->blockSize = 0;

        configureFile->maxBlockNum = 0;

        configureFile->filePath = "/service/";

        configureFile->port = 1234;

        configureFile->ip = "127.0.0.1";

        configureFile->worker = 1;
    }
    //printfConfigure();
}

void ConfigureManager::printfConfigure()
{
    if ( configureFile )
    {

        cout << "configureFile->blockSize=" << configureFile->blockSize << endl;

        cout << "configureFile->maxBlockNum=" << configureFile->maxBlockNum << endl;

        cout << "configureFile->filePath=" << configureFile->filePath << endl;

        cout << "configureFile->port=" << configureFile->port << endl;

        cout << "configureFile->ip=" << configureFile->ip << endl;

        cout << "configureFile->worker=" << configureFile->worker << endl;

        cout << "configureFile->levels=" << configureFile->levels << endl;

        cout << "configureFile->diskSize=" << configureFile->diskSize << endl;
        cout << "configureFile->maxPiece=" << configureFile->maxPiece << endl;



        LOG_INFO(__FILE__, __LINE__, "cache 块大小(单位是字节):%u", configureFile->blockSize);
        LOG_INFO(__FILE__, __LINE__, "cache 块数量:%u", configureFile->maxBlockNum);
        LOG_INFO(__FILE__, __LINE__, "cache文件存放的路径:%s", (configureFile->filePath).c_str());
        LOG_INFO(__FILE__, __LINE__, "cache 服务器的端口是:%u", configureFile->port);
        LOG_INFO(__FILE__, __LINE__, "cache 服务器的ip地址是:%s", (configureFile->ip).c_str());
        LOG_INFO(__FILE__, __LINE__, "cache 线程池的数量:%u", configureFile->worker);
        LOG_INFO(__FILE__, __LINE__, "hash文件的规则:%s", (configureFile->levels).c_str());
        LOG_INFO(__FILE__, __LINE__, "本地磁盘分块的大小:%u", configureFile->diskSize);
        LOG_INFO(__FILE__, __LINE__, "最大块的大小:%u", configureFile->maxPiece);



    }
}
//
/*void ConfigureManager::writeSomething(char *fileName)
{
    parsexml parseConfig(fileName,configureFile);

}*/

shared_ptr <configureInfo>  ConfigureManager::readConfigure()
{
    return configureFile;
}



ConfigureManager* ConfigureManager::getInstance()
{
    static ConfigureManager confiureInstance;

    return &confiureInstance;
}
//结束配置类的实现
// 客户端事件管理 类实现
/*
UserManager::~UserManager()
{
    ~Manager();
}*/

//结束用户管理类的配置

//开始实现IO类

IOManager::IOManager()
{

}


IOManager* IOManager::getInstance()
{
    static IOManager ioInstance;

    return & ioInstance;
}


bool IOManager::AIORead2( const char* fileName, unsigned int &index, shared_ptr <configureInfo> configureContent, unsigned  int& g_Info, LinkListNode* guardNode, bool flag )
{
    string fileTrans = configureContent->filePath;
    string hashFile = fileName;
    unsigned int oldoffset = index * ( configureContent->blockSize );
    if ( !parseHash( configureContent->levels, fileTrans, hashFile ) )
    {
        g_Info = 901; //hash文件解析错误
        //modfd( m_epollfd, m_sockfd, EPOLLIN );
        //return;
        return false;
    }
    oldoffset = diskInstance->getRealFile2(fileTrans, oldoffset);
    const char *file = fileTrans.c_str();
    int fd, ret = -1;
    char* buf = nullptr;


    io_context_t ctx;
    struct iocb  cb;
    struct iocb *cbs[1];
    struct io_event events[1];
    //char file[50];

    //strcpy( file, ( configureContent->filePath ).c_str() );
    //strcat( file, fileName );
#ifdef __CACHE_DEBUG
    cout << "开启Native aio 读取文件:" << file << endl;
#endif
#ifdef __LOG_DEBUG
    LOG_INFO(__FILE__, __LINE__, "开启Native aio 读取文件");
#endif
    fd = open( file, O_RDONLY | O_DIRECT ); //O_DIRECT开启这个才是异步，不开启这个是同步

    if ( fd == -1 )
    {
        g_Info = 700;
#ifdef __CACHE_DEBUG
        cout << "打开文件错误" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "Native aio 读取文件时,打开文件%s发生错误", file);
#endif
        goto err;
    }

    //buf = ( char* )aligned_alloc( 512, configureContent->blockSize ); // aligned_alloc() 申请存放读取内容的 buffer，起始地址需要和磁盘逻辑块大小对齐（一般是 512 字节）。
    ret = posix_memalign((void **)&buf, 512, configureContent->blockSize);
    if (ret < 0)
    {
        //close( fd );

        g_Info = 800;
#ifdef __CACHE_DEBUG
        cout << "服务器空间申请失败" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "服务器空间申请失败");
#endif

        goto err1;

    }

    memset( &ctx, 0, sizeof( ctx ) ); //在调用该函数之前必须将*ctxp初始化为0

    ret = io_setup( NR_EVENT, &ctx );


    if ( ret != 0 )
    {

        //close( fd );
#ifdef __CACHE_DEBUG
        cout << "io_setup failed:" << errno << "\n";
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "native aio io_setup failed");
#endif
        g_Info = 900;

        goto err2;
    }
    memset(buf, 0, configureContent->blockSize);
    memset( &cb, 0, sizeof( cb ) );
    io_prep_pread(&cb, fd, buf, configureContent->blockSize, oldoffset);
    /*
    cb.aio_data = ( __u64 )buf;

    cb.aio_fildes = fd;

    cb.aio_lio_opcode = IOCB_CMD_PREAD;

    cb.aio_buf = ( __u64 )buf;
    //cout<<"index="<<index<<endl;
    cb.aio_offset = ( __s64 )(oldoffset);
    //cout<<"cb.aio_offset="<<cb.aio_offset<<endl;

    cb.aio_nbytes = configureContent->blockSize;
    在io_prep_pread中全部设置完成
    */
    //cb.data =
    cbs[0] = &cb;

    ret = io_submit(ctx, 1, cbs);

    if ( ret != 1 )
    {

        //free( buf );

        //close( fd );
        g_Info = 900;
#ifdef __CACHE_DEBUG
        cout << "io_submit error:" << errno << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "native aio io_submit error");
#endif
        goto err3;
    }


    ret = io_getevents( ctx, 1, 1, events, nullptr ); //阻塞等待

    if ( ret != 1 )
    {


#ifdef __CACHE_DEBUG
        cout << "io_getevents error:" << errno << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "native aio io_getevents error");
#endif

        g_Info = 900;

        goto err3;
    }

    if ( events[0].res<= 0 )
    {
#ifdef __CACHE_DEBUG
        cout << "io error:" << events[0].res << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "读取文件，数据长度(如果为0,就是读越界)：", events[0].res);
#endif
        g_Info = 401;

        goto  err3;
    } else
    {

        if ( flag ) //当使用哨兵的时候要用
        {
            mutex_Lock.lock();
            guardNode->name  =  hashFile;
            guardNode->offset = index;
        }
        memcpy( guardNode->data, ( const void* )( buf ), events[0].res );
        guardNode->length = events[0].res;
        io_destroy( ctx );
        free( buf );
        close( fd );
#ifdef __CACHE_DEBUG
        if ( events[0].res < configureContent->blockSize )
        {
            cout << "WARNING!!!读出了" << events[0].res << "个字节，但是块的大小是：" << configureContent->blockSize << "字节，这个原因可能也是因为文件的末尾" << endl;
        }
        /*
        else if( event.res >= configureContent->blockSize)
        {
            cout<<"res= "<<event.res<<",blockSize="<<configureContent->blockSize<<endl;
        }
        */
#endif
#ifdef __LOG_DEBUG
        if ( events[0].res < configureContent->blockSize )
        {
            LOG_WARNING(__FILE__, __LINE__, "读取的信息无法填满整个cache块", events[0].res);
        }
#endif
        return true;
    }
err3:
    ret  =  io_destroy( ctx );
    if (ret < 0)
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "io_destroy");
#endif
err2:
    free( buf );
    buf = nullptr;
err1:
    close(fd);
err:
    return false;
}

}
