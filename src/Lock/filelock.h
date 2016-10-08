//#include <unistd.h>
//#include <fcntl.h>
#ifndef FILELOCK_H
#define FILELOCK_H
#include <sys/file.h>
//文件锁
namespace cachemanager {

inline int fileWrlock(FILE *fp)
{
	return  flock( fileno( fp ), LOCK_EX );
}

inline int fileRdlock(FILE *fp)
{
	return flock(fileno(fp), LOCK_SH);
}


inline void fileUnlock(FILE *fp)
{
	int result = flock( fileno( fp ), LOCK_UN );

	fclose(fp);
	if (result == -1)
	{

		perror( "flock()" );
	}
	//return  result;
}
}
#endif 