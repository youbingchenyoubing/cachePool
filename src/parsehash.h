# ifndef PARSEHASH_H
# define PARSEHASH_H
# include <unistd.h>
# include <string>
# include <string.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <dirent.h>
#ifdef __LOG_DEBUG
# include "./Log/Log.h"
#endif
#ifdef  __CACHE_DEBUG
# include <iostream>
#endif
using namespace std;
namespace cachemanager
{
bool  parseHash( string rule, string& fileName, const string& hashName );
bool  makeDir( string& path );
bool  openNewFile(string rule, string& fileName, const string & hashName, const unsigned int &fileSize);
bool  touchFile(const string & fileName, const unsigned int & fileSize);
bool changeFileSize(const char *  file, const unsigned int & fileSize);
bool fileExitOrNot(const char *file);

}
#endif