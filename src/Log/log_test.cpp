#include <time.h> //clock_t,clock(),CLOCKS_PER_SEC


#include <pthread.h>//pthread_create



#include "Log.h"
using namespace cachemanager;

int main()
{

    SET_LOG_NAME("/root/Log/test.log");

    //SET_LOG_NAME("C:\\test\\test.log");

    //设置日志级别
    SET_LOG_LEVEL(LOG_LEVEL_TRACE);

    //设置日志大小
    SET_LOG_SIZE(10 * 1024 * 1024);

    //设置占用磁盘空间大小
    SET_LOG_SPACE(100 * 1024 * 1024);

    clock_t start, finish;
    double duration;

    start = clock();

    for(int i = 0; i < 1000000; i++)
    {
        LOG_TRACE(__FILE__,__LINE__,"****%d****", i);
        LOG_INFO(__FILE__,__LINE__,"test INFO");
        LOG_WARNING(__FILE__,__LINE__,"test WARNING");
        LOG_ERROR(__FILE__,__LINE__,"test ERROR");
    }

    finish = clock();
    duration = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("duration = %.3f.\n", duration);

    getchar();

    return 0;
}

