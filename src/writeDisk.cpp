# include "writeDisk.h"
# include <cstddef>
namespace cachemanager
{
DiskManager* DiskManager::getInstance( const size_t& ds, const unsigned int & blockSize )
{
    static DiskManager m_instance( ds, blockSize );

    return &m_instance;
}
bool DiskManager::openNewFile(const string & hashName, unsigned int& filesize)
{
    unsigned int blockCount = filesize / diskSize; //将已知大小的文件分成几块
    if (filesize % diskSize) blockCount++;
    unsigned int oneBlockSize;
    string str;
    string temp;
    stringstream ss;
    //char str2[10];
    for (unsigned int i = 0; i < blockCount; ++i)
    {
        if (i == blockCount - 1)
            oneBlockSize = filesize - i * diskSize; //最后一块
        else oneBlockSize = diskSize;
        str = "";
        temp = "";
        ss.clear();
        ss << i;
        ss >> temp;
        str = hashName + ".t" + temp;
        if (!touchFile(str, oneBlockSize))
        {
#ifdef __CACHE_DEBUG
            cout << "创建文件:" << str << "失败" << endl;
#endif
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "创建文件失败:%s", str.c_str());
#endif
            return false;

        }

    }
    return true;

}
bool DiskManager::writeOneBuff2(const string& hashName, const char buff[], const unsigned int& offset, unsigned int& length)
{
    unsigned int newoffset = offset % diskSize;
    if (!touchFile(hashName, diskSize))  //已经有了就 不创建
    {
#ifdef __CACHE_DEBUG
        cout << "创建文件:" << hashName << "失败" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "创建文件失败:%s", hashName.c_str());
#endif
        return false;
    }
    FILE* fp = fopen( hashName.c_str(), "rb+" ); //二进制写文件

    if ( fp == NULL )
    {
        //因为上面建文件的时候，没有加锁，对于多线程很可能会有问题，不过这个小概率事件。
#ifdef __CACHE_DEBUG
        cout << "errno=" << errno << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "文件打开失败，错误代码是:%d", errno);
#endif
        //freeNew( str );
        return false;
    }
    if (fileWrlock(fp) == -1)
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!文件加锁失败" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "文件加锁失败");
#endif
        return false;
    }
    //fseek( fp, diskSize - 1, SEEK_SET );
    fseek( fp, newoffset, SEEK_SET );

    if ( fwrite( buff, sizeof( char ), length, fp ) == length )
    {

        fileUnlock(fp);

        //fclose( fp );
        return  true;
    }
    else
    {

        fileUnlock(fp);
        return false;
    }
//e

}
bool DiskManager::writeOneBuff( const string& hashName, const char buff[], const unsigned int& offset, unsigned int& length )
{


    unsigned int beginBlock = offset / diskSize;
    //cout<<beginBlock<<endl;

    unsigned int newoffset =  offset % ( diskSize );

    unsigned int endBlock = ( offset + length ) / diskSize;

    //int filelength = hashName.length();

    string str;
    string temp;
    stringstream ss;
    //char str2[10];
    //strcpy(str,hashName.c_str());
    unsigned int len1;

    for ( unsigned int i = beginBlock; i <= endBlock; ++i )
    {


        len1 = ( diskSize - newoffset < length ? diskSize - newoffset : length );
        //memset( str, '\0', filelength + 10 );
        str = "";
        temp = "";
        ss.clear();
        ss << i;
        ss >> temp;
        str =  hashName + ".t" + temp;

        //snprintf( str, filelength + 10, "%s.t%u", hashName.c_str(), i );
        if (!touchFile(str, diskSize))
        {
#ifdef __CACHE_DEBUG
            cout << "创建文件:" << str << "失败" << endl;
#endif
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "创建文件失败:%s", str.c_str());
#endif
            return false;
        }

        //isFirst = true;
        FILE* fp = fopen( str.c_str(), "rb+" ); //二进制写文件

        if ( fp == NULL )
        {
            //因为上面建文件的时候，没有加锁，对于多线程很可能会有问题，不过这个小概率事件。
#ifdef __CACHE_DEBUG
            cout << "errno=" << errno << endl;
#endif
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "文件打开失败，错误代码是:%d", errno);
#endif
            //freeNew( str );
            return false;
        }
        if (fileWrlock(fp) == -1)
        {
#ifdef __CACHE_DEBUG
            cout << "ERROR!!!文件加锁失败" << endl;
#endif
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "文件加锁失败");
#endif
            return false;
        }
        //fseek( fp, diskSize - 1, SEEK_SET );
        fseek( fp, newoffset, SEEK_SET );

        if ( fwrite( buff, sizeof( char ), len1, fp ) == len1 )
        {

            fileUnlock(fp);

            //fclose( fp );
            newoffset = 0;
            length -= len1;
        }
        else
        {

            fileUnlock(fp);
            return false;
        }
    } //end  for
//freeNew( str );
    if ( length == 0 )
    {
        return true;
    }
    else
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!计算出错了" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "写文件时候，计算出错了");
#endif
        return false;
    }


}
bool DiskManager::writeAllBuff(const string& hashName, const char buff[], const unsigned int& offset, unsigned int& length)
{
    bool result = true;
    const char * file = hashName.c_str();
    FILE* fp = fopen( file, "rb+" );
    if (fp == NULL)
    {
#ifdef __CACHE_DEBUG
        cout << "文件:" << file << "不存在" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "文件不存在%s", file);
#endif
        return false;
    }
    if ( flock( fileno( fp ), LOCK_EX ) < 0 ) //建立互斥锁，写文件严格控制一致性
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!文件加锁失败" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "文件加锁失败");
#endif
        return false;
    }
    fseek( fp, offset, SEEK_SET );
    if ( fwrite( buff, sizeof( char ), length, fp ) != length )
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!长度不一致" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "写长度不一致");
#endif
        //fclose(fp);
        //return false;
        result = false;
    }
    if ( flock( fileno( fp ), LOCK_UN ) == -1 ) //释放文件锁
    {
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "解锁失败");
#endif
        perror( "flock()" );
        result = false;
    }
    fclose(fp);
    return result;
    //return  true;
}
// 根据偏移量的到真正的分块
unsigned  int DiskManager::getRealFile(string & hashName, unsigned int &offset)
{
    unsigned int  index = offset / diskSize;
    //string str;
    string temp;
    stringstream ss;
    ss.clear();
    ss << index;
    ss >> temp;
    hashName = hashName + ".t" + temp;
    return index;
}
//根据偏移量得到真正的偏移量
unsigned int DiskManager::getRealFile2(string & hashName,unsigned int & offset)
{
    unsigned int index = offset /diskSize;

    string temp;
    stringstream ss;
    ss.clear();
    ss << index;
    ss >> temp;
    hashName = hashName + ".t" + temp;

    index =  offset % diskSize;
    return index;

}
bool DiskManager::checkWrite(const string &  hashName, unsigned int & offset, unsigned int & length, unsigned int &code, unsigned int &filesize)
{
    const char * file = hashName.c_str();
    if (access(file, F_OK) == -1)
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!文件:" << file << "不存在" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "文件:%s不存在", file);
#endif
        code = 400;
        return false;
    }
    filesize = getFileSize(file);
    if (filesize <= offset || filesize < (offset + length))
    {

#ifdef __CACHE_DEBUG
        cout << "ERROR!!!写文件越界了" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "写文件越界了，文件大小为%u,偏移量为:%u,最后一个长度为:%u", filesize, offset, offset + length);
#endif
        code =  401;
        return false;

    }
    return true;
}
//获取文件的大小
unsigned int DiskManager::getFileSize(const char *file)
{
    if(access(file,F_OK)==-1)
        return diskSize;
    struct stat temp;
    stat(file, &temp);
    return temp.st_size;
}
//释放空间
void DiskManager::freeNew( char* str )
{
    if ( str )
    {
        delete [] str;
        str = NULL;
    }

}
}