// 这是一个管理磁盘文件块的位图类

#ifndef BITMAP_H
#define BITMAP_H
#ifdef __CACHE_DEBUG
# include <stdio.h>
#endif
#include <unistd.h>
#ifdef __LOG_DEBUG
# include "./Log/Log.h"
#endif
//# include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
# include <errno.h>
# include <iostream>
# include <string>
# include <memory.h>
//# include "locker.h"
using namespace std;
/*
# include <bitset>
*/
//# include "locker.h"
//static const unsigned  int MAX_PIECE = 1000000; //最多的块，这个是为了方便位图的初始化
namespace cachemanager
{
class  BitMap
{
public:
    BitMap( const string & hashName, const unsigned int& MAX_PIECE1);
    int getByteValue( unsigned int index );
    int setByteValue( unsigned int index, unsigned
                      char v );
    //bool all_zero();
    //bool all_set();
    bool isChanged;
    const string bitFile; //这个类存的那个分块的位图信息
    bool restoreBitMap();
    ~BitMap();
    void  printfBitMap(); //打印位图测试接口
private:
    const unsigned int MAX_PIECE;
    unsigned char* bitfiled;  //位图
    unsigned int bit_size; //多少个bit



};
//上面那个版本有点问题
/*
class BitMap2{
  public:
    BitMap2(const char *hashName,const unsigned int & MAX_PIECE1);
    int getByteValue(unsigned int index);
    void setByteValue(unsigned int index,bool flag);

    bool restoreBitMap(const char *hashName);
    ~BitMap2();
    void  printfBitMap();
  private:
    static  const unsigned int MAX_PIECE = 100;
    std::bitset<MAX_PIECE> bitfiled;
};
*/
}
#endif