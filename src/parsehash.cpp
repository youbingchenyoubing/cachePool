# include "parsehash.h"

namespace cachemanager
{
bool  parseHash( string rule, string& fileName, const string& hashName )
{

    string::size_type pos;

    rule += ":";

    unsigned int  size = rule.size();
    unsigned int begin = 0;
    unsigned int end = 0;
    unsigned  int length = hashName.length();
    for ( unsigned int i = 0; i < size; i++ )
    {
        pos = rule.find( ":", i );

        if ( pos < size )
        {
            std::string s = rule.substr( i, pos - i );
            end = atoi( s.c_str() );
            if ( ( begin + end ) >= length )
            {
                end = length - begin; //如果level解析太长，将剩余所有的长度作为最后一个
                //return false;
            }
            fileName += hashName.substr( begin, end );
            if ( pos != size - 1 )
            {

                if ( !makeDir( fileName ) )
                {
#ifdef __CACHE_DEBUG
                    cout << "ERROR!!!创建目录失败" << endl;
#endif
#ifdef __LOG_DEBUG
                    LOG_ERROR(__FILE__, __LINE__, "创建目录失败");
#endif
                    return false;
                }
                fileName += "/";
                // 这个是目录

            }
            begin += end;
            i = pos;
        }
    }
    return true;
}
//创建目录
bool makeDir( string& path )
{

    const char* paths = path.c_str();
    if ( NULL == opendir( paths ) ) //目录不存在
    {
        //创建目录
        int ret  = mkdir( paths, S_IRWXU | S_IRGRP | S_IWGRP );

        if ( ret != 0 )
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    return true;
}
//创建文件入口函数
bool openNewFile(string rule, string& fileName, const string & hashName, const unsigned int &fileSize)
{
    string::size_type pos;

    rule += ":";

    unsigned int  size = rule.size();
    unsigned int begin = 0;
    unsigned int end = 0;
    unsigned  int length = hashName.length();
    for ( unsigned int i = 0; i < size; i++ )
    {
        pos = rule.find( ":", i );

        if ( pos < size )
        {
            std::string s = rule.substr( i, pos - i );
            end = atoi( s.c_str() );
            if ( ( begin + end ) >= length )
            {
                end = length - begin;
                //return false;
            }
            fileName += hashName.substr( begin, end );
            if ( pos != size - 1 )
            {

                if ( !makeDir( fileName ) )
                {
#ifdef __CACHE_DEBUG
                    cout << "ERROR!!!创建目录失败" << endl;
#endif
#ifdef __LOG_DEBUG
                    LOG_ERROR(__FILE__, __LINE__, "创建目录失败");
#endif
                    return false;
                }
                fileName += "/";
                // 这个是目录

            }
            begin += end;
            i = pos;
        }
    } //end  for ( unsigned int i = 0; i < size; i++ )
    if (!touchFile(fileName, fileSize))
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!创建文件失败" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "创建文件失败");
#endif
    }
    return true;

}
//创建固定大小的文件
bool touchFile(const string & fileName, const unsigned int & fileSize)
{
    const char * file = fileName.c_str();
    if (access(file, F_OK) == -1) //文件不存在
    {
        FILE * fp = fopen(file, "a");
        if (fp == NULL)
        {
#ifdef __CACHE_DEBUG
            cout << "ERROR!!!创建磁盘块文件失败" << endl;
#endif
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "创建磁盘块文件失败");
#endif
            return false;
        }
        fclose(fp);
        if (!changeFileSize(file, fileSize))
        {
#ifdef __CACHE_DEBUG
            cout << "ERROR!!!创建磁盘块文件失败" << endl;
#endif
#ifdef __LOG_DEBUG
            LOG_ERROR(__FILE__, __LINE__, "创建磁盘块文件失败");
#endif

            remove(fileName.c_str());
            string file = fileName+".bitmap";
            remove(file.c_str());
            return false;
        }
    } //end if (access(file,F_OK))
    //code = 408;
    return true;
}
// 改变文件大小
bool changeFileSize(const char *  file, const unsigned int & fileSize)
{
    if (truncate(file, fileSize) == -1)
    {
#ifdef __CACHE_DEBUG
        cout << "ERROR!!!指定文件大小失败" << endl;
#endif
#ifdef __LOG_DEBUG
        LOG_ERROR(__FILE__, __LINE__, "指定文件大小失败,文件大小是:%u", fileSize);
#endif
        //code = 40
        return false;
    }
    return true;
}
// 查询文件是否存在

bool fileExitOrNot(const char *file)
{
    if (access(file, F_OK) == -1)
    {
        return false;
    }
    return  true;
}
} // end  namespace




