#ifndef LOG_H
#define LOG_H
//日志文件
#include <stdio.h>
#include <vector>
#include <string>
#include "../locker.h"

using namespace std;
namespace  cachemanager
{

//单实例API
#define DEFAULT_MODULE                      "DEFAULT"
#define SET_LOG_NAME(filepath)              CLogFactory::get_instance(DEFAULT_MODULE)->set_log_filepath(filepath)
#define SET_LOG_LEVEL(level)                CLogFactory::get_instance(DEFAULT_MODULE)->set_log_level(level)
#define SET_LOG_SIZE(size)                  CLogFactory::get_instance(DEFAULT_MODULE)->set_log_size(size)
#define SET_LOG_SPACE(size)                 CLogFactory::get_instance(DEFAULT_MODULE)->set_log_space(size)

#define LOG_TRACE(file_code,line,format, ...)              CLogFactory::get_instance(DEFAULT_MODULE)->writeline(LOG_LEVEL_TRACE,file_code,line,format, ## __VA_ARGS__)
#define LOG_INFO(file_code,line, format,...)               CLogFactory::get_instance(DEFAULT_MODULE)->writeline(LOG_LEVEL_INFO,file_code,line,format, ## __VA_ARGS__)
#define LOG_WARNING(file_code,line,format, ...)            CLogFactory::get_instance(DEFAULT_MODULE)->writeline(LOG_LEVEL_WARNING,file_code,line, format, ## __VA_ARGS__)
#define LOG_ERROR(file_code,line,format, ...)              CLogFactory::get_instance(DEFAULT_MODULE)->writeline(LOG_LEVEL_ERROR,file_code,line, format, ## __VA_ARGS__)


typedef unsigned int uint;
typedef unsigned long uint32;
typedef unsigned long long uint64;

//日志级别，可以在配置文件进行配置
enum _log_level {
	LOG_LEVEL_ERROR = 1, //错误
	LOG_LEVEL_WARNING = 2, //警告
	LOG_LEVEL_INFO = 3, //普通
	LOG_LEVEL_TRACE = 4, //调试
	LOG_LEVEL_MAX
};


#define MAX_TIME_STR_LEN 30
/*日志文件的信息*/
typedef struct
{
	string file_path;  //文件路径
	uint64 size;      // 文件大小
	time_t mtime;    //修改时间
} file_info;

typedef vector<file_info> file_list;
class  CLog {
public:
	CLog(void);
	~CLog(void);
	//设置日志文件的各种属性
	//设置日志文件路径
	void set_log_filepath(const string & file_path) {m_log_filename =  file_path; mk_dir(); rotate_log();}

	//设置日志文件大小，单位字节

	void  set_log_size(const uint & size) {m_log_size = size;}

	//设置日志文件占用磁盘空间大小，单位字节
	void set_log_space(const uint64 & size) {m_max_log_space  = size;}

	//设置日志级别
	void set_log_level(const uint & level) {m_log_level = ((level > LOG_LEVEL_MAX) ? LOG_LEVEL_MAX : level);}

	//设置清除状态
	void set_clean_status(const bool &  status) {m_lock.lock(); m_is_cleaning = status; m_lock.unlock();}

	const  string & get_log_path() {return m_log_filename;}

	const  uint64 & get_max_space() {return m_max_log_space;}

	//写入一行日志
	int writeline(uint level, const string & file_code, int line, const char* str, ...);

private:
	void  rotate_log();    //日志切换
	bool  mk_dir();   //检查目录是否存在，不存在就循环创建
	char  * get_time_str(bool is_write = true); //获取当前时间字符串
	bool rename_file(); //重名文件
	int get_thread_id(); //获取线程id
	void clean_logs(); // 清理日志文件

	static void * clean_logs_thread_fun(void *  param); //日志文件清理线程

	static bool get_log_files(const string & log_path, file_list & files); //根据日志文件名获取相关日志列表
	//根据日志文件名获取日志目录(末尾带有'\'或'/')
	static string get_log_dir(const string & log_path);

	locker m_lock; //同步锁,用于多线程同步写

	string m_log_filename; //日志文件路径名

	FILE * m_fp;    //日志文件句柄

	uint   m_log_level; //设置日志级别

	uint   m_log_size; //设置日志文件大小

	uint64 m_max_log_space; //设置日志文件占用磁盘的大小

	bool m_is_cleaning; //是否正在进行日志清理

	char m_time_str[MAX_TIME_STR_LEN];//时间字符串

};

#define MAX_LOG_INSTANCE    10
#define MAX_MODULE_LEN      30

typedef struct _log_instance
{
	char  name[MAX_MODULE_LEN];
	CLog* plog;
} log_inst;

class CLogFactory
{
public:
	CLogFactory(void);
	virtual ~CLogFactory(void);

	static CLog* get_instance(const char* name);
	static void  free_instance(const char* name);

private:
	static void  free_all_inst();

private:
	static log_inst     m_inst_list[MAX_LOG_INSTANCE];      //log实例列表
	static uint         m_inst_num;                         //log实例数量
	static locker       m_lock;                             //同步锁
};
/*
#ifdef SINGLE_LOG
#define DEFAULT_MODULE                      "DEFAULT"
inline void SET_LOG_NAME(string filepath) {
	CLogFactory::get_instance(DEFAULT_MODULE)->set_log_filepath(filepath);
}
inline  void SET_LOG_LEVEL(uint level) {
	CLogFactory::get_instance(DEFAULT_MODULE)->set_log_level(level);
}

inline void  SET_LOG_SIZE(uint size) {
	CLogFactory::get_instance(DEFAULT_MODULE)->set_log_size(size);
}
inline void SET_LOG_SPACE(uint64 size) {
	CLogFactory::get_instance(DEFAULT_MODULE)->set_log_space(size);
}

inline void  LOG_TRACE(const char * format, ...) {
	CLogFactory::get_instance(DEFAULT_MODULE)->writeline(LOG_LEVEL_TRACE, format, ## __VA_ARGS__);
}
inline void LOG_INFO(const char * format, ...) {
	CLogFactory::get_instance(DEFAULT_MODULE)->writeline(LOG_LEVEL_INFO, format, ## __VA_ARGS__);
}
inline void LOG_WARNING(const  char *format, ...) {
	CLogFactory::get_instance(DEFAULT_MODULE)->writeline(LOG_LEVEL_WARNING, format, ## __VA_ARGS__);
}
inline void LOG_ERROR(const char *format, ...) {
	CLogFactory::get_instance(DEFAULT_MODULE)->writeline(LOG_LEVEL_ERROR, format, ## __VA_ARGS__);
}

#else
//多实例API
inline SET_LOG_NAME(char *module, string filepath) {
	CLogFactory::get_instance(module)->set_log_filepath(filepath);
}

inline SET_LOG_LEVEL(char *module, uint level) {
	CLogFactory::get_instance(module)->set_log_level(level);
}

inline SET_LOG_SIZE(char *module, uint  size) {
	CLogFactory::get_instance(module)->set_log_size(size);
}
inline SET_LOG_SPACE(char *module, uint64 size) {
	CLogFactory::get_instance(module)->set_log_space(size);
}
inline LOG_TRACE(char * module, const char *format, ...) {

	CLogFactory::get_instance(module)->writeline(LOG_LEVEL_TRACE, format, ## __VA_ARGS__);
}

inline LOG_INFO(char *module, const char *format, ...) {
	CLogFactory::get_instance(module)->writeline(LOG_LEVEL_INFO, format, ## __VA_ARGS__);
}

inline LOG_WARNING(char *module, const char * format, ...) {
	CLogFactory::get_instance(module)->writeline(LOG_LEVEL_WARNING, format, ## __VA_ARGS__);
}

inline LOG_ERROR(char *module, const char *format, ...) {
	CLogFactory::get_instance(module)->writeline(LOG_LEVEL_ERROR, format, ## __VA_ARGS__);
}
#endif*/
}
#endif