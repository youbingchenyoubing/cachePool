# include "bitmap.h"
//#include <fcntl.h>

namespace cachemanager
{
BitMap::BitMap( const string & hashName, const unsigned int& MAX_PIECE1): MAX_PIECE( MAX_PIECE1 ),isChanged(false),bitFile(hashName)
{
    //MAX_PIECE = MAX_PIECE1;
    bit_size = MAX_PIECE / 8;
    if ( MAX_PIECE % 8 )
    {
        bit_size++;
    }
    bitfiled = new unsigned char[bit_size];

    if ( !bitfiled )
    {
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "内存分配错误");
#endif
        perror( "malloc error" );
    }
    /*
    fstream openhandler;
    openhandler.open(hashName,ios::in);
     */
    FILE* fp = fopen( hashName.c_str(), "r" );

    if ( fp == NULL ) //打开文件失败，说明开始是一个全新的下载
    {
        memset( bitfiled, 0, bit_size );
    }
    else
    {

        fseek( fp, 0, SEEK_SET );
        for ( unsigned  int i = 0; i < bit_size; ++i )
        {
            bitfiled[i] = fgetc( fp );
        }

        fclose( fp );
    }

}
/*取某一个index的位图*/
int BitMap::getByteValue( unsigned int index )
{

    unsigned int byte_index;
    unsigned char inner_byte_index;

    unsigned char byte_value;
    //cout<<"MAX_PIECE="<<MAX_PIECE<<endl;
    if ( index >= MAX_PIECE )
    {
#ifndef  __CACHE_DEBUG
        cout << "ERROR!!!,上传的文件过大，拒绝上传" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "上传的文件过大，拒绝上传");
#endif
        return -1;
    }

    byte_index = index / 8;
    byte_value = bitfiled[byte_index];
    inner_byte_index = index % 8;

    byte_value = byte_value >> ( 7 - inner_byte_index );


    if ( byte_value % 2 == 0 )
    {
        return 0;
    }

    else
    {
        return 1;
    }
}


int BitMap::setByteValue( unsigned int index, unsigned char v )
{
    int byte_index;
    unsigned char inner_byte_index;


    if ( index > MAX_PIECE )
    {
#ifndef  __CACHE_DEBUG
        cout << "ERROR!!!,上传的文件过大，拒绝上传" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "上传的文件过大，拒绝上传");
#endif
        return -1;
    }

    if ( ( v != 0 ) && ( v != 1 ) )
    {
#ifndef  __CACHE_DEBUG
        cout << "ERROR!!! v的值出错了!!!" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "位图设置值出错");
#endif
        return -1;
    }

    byte_index = index / 8;
    inner_byte_index = index % 8;
    v = v << ( 7 - inner_byte_index );
    bitfiled[byte_index] = bitfiled[byte_index] | v;
    //printf( "%.2X ", bitfiled[byte_index] );
    isChanged = true;
    return 0;
}

bool BitMap::restoreBitMap()
{
    //int fd;
    const char* hashName = bitFile.c_str();
    bool isFirst = false;
    if ( bitfiled == NULL || hashName == NULL )
    {
        return false;
    }

    /*没有互斥创建文件，高并发可能有点bug*/
    if ( access( hashName, F_OK ) == -1 ) //说明是第一位图
    {
        FILE* tempfp = fopen( hashName, "a" );
        if ( tempfp == NULL )
        {
#ifdef __CACHE_DEBUG
            cout << "ERROR!!!创建位图文件失败" << endl;
#endif
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "创建位图文件失败");
#endif
            return false;
        }
        fclose( tempfp );
        isFirst = true;
    }

    FILE* fp = fopen( hashName, "rb+" );
    if ( fp == NULL )
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!更新位图文件失败" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "更新位图文件失败");
#endif
        return false;

    }
    if ( flock( fileno( fp ), LOCK_EX ) < 0 ) //建立互斥锁，以免更新位图的时候，有线程进行读取
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!位图文件加锁失败" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "位图文件加锁失败");
#endif
        return false;
    }
    /*写之间先于文件异或一下，为了防止多线程写,出现脏写，这个过程有点像数据库的乐观锁*/
    fseek( fp, 0, SEEK_SET );
    if ( !isFirst )
    {
        //unsigned char temp;
        /*如果不是第一个位图信息，要与之前位图信息异或*/
        for ( unsigned  int i = 0; i < bit_size; ++i )
        {
            //temp = fgetc(fp);
            //printf("%.2X ",bitfiled[i]);
            //printf("%.2X ",temp);
            bitfiled[i] = bitfiled[i] | fgetc( fp );
        }

        fseek( fp, 0, SEEK_SET );
    }
    fwrite( bitfiled, sizeof( char ), bit_size, fp );
    if ( flock( fileno( fp ), LOCK_UN ) == -1 ) //释放文件锁
    {
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "解锁失败");
#endif
        perror( "flock()" );
    }
    fclose( fp );
    return true;
}

void BitMap::printfBitMap()
{
    for ( unsigned int i = 0; i < bit_size; ++i )
    {
        printf( "%.2X ", bitfiled[i] );
        if ( ( i + 1 ) % 16 == 0 )
        {
            printf( "\n" );
        }
    }
    printf( "\n" );

}
BitMap::~BitMap()
{
    if ( bitfiled )
    {
        delete bitfiled;
#ifdef __CACHE_DEBUG
        cout << "释放位图空间" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_INFO(__FILE__, __LINE__, "释放位图空间");
#endif
    }

}
}

