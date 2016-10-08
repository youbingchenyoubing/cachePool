#ifndef WRITEDISK_H
#define WRITEDISK_H
# include <iostream>
# include <memory.h>
# include <sys/types.h>
#include <sys/stat.h>
# include <sys/file.h>
#include <sstream>
#include <fcntl.h>
# include <string>
#include <unistd.h>
# include "parsehash.h"
# include "./Lock/filelock.h"
#ifdef __LOG_DEBUG
# include "./Log/Log.h"
#endif

using namespace std;
//static ConfigureManager * configureInstance;
namespace cachemanager
{
class DiskManager
{
public:
    //不管已知大小是否已知，都直接分块写文件
    bool writeOneBuff2(const string& hashName, const char buff[], const unsigned int& offset, unsigned int& length);
    bool writeOneBuff( const string& hashName, const char buff[], const unsigned int& offset, unsigned int& length );
    static DiskManager* getInstance( const size_t& ds,const unsigned int & blockSize );
    //面对已知大小的写文件
    bool writeAllBuff(const string& hashName, const char buff[], const unsigned int& offset, unsigned int& length);
    bool checkWrite(const string &  hashName, unsigned int & offset, unsigned int & length, unsigned int &code, unsigned int & filesize);
    //分块创建已知大小的的文件
    bool openNewFile(const string & hashName,unsigned int& filesize);
    unsigned int getFileSize(const char *file);
    unsigned int getRealFile(string & hashName, unsigned int &offset);
    unsigned int getRealFile2(string & hashName, unsigned int &offset);
    const  unsigned int   times;
private:
    /*begin 和end 通过计算得到的跨越块的情况*/
    //unsigned int begin;
    //unsigned int end;
    const  unsigned int   diskSize;
    //文件分块，一块是cache块大小的几倍
    DiskManager( const size_t& ds, const unsigned int & blockSize): times(ds),diskSize( ds * blockSize )
    {
        //cout<<"ds="<<ds<<endl;
    }
    void freeNew( char* str );
    //获得文件的属性
    //static DiskManager * m_Instance;
    //这个是至关重要的

};
}
#endif