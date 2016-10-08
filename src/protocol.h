#ifndef PROTOCOL_H
#define PROTOCOL_H
#define ERRORHEADER 100 //客户发送过来的头部是错误的
#define ERRORBODY   101 //客户发送过来的内容是错误的
#define CACHEMISS   102  //发送某一部分内容
#define OKR         200 //完全读成功
#define OKW         300  // 写成功
# define BADBITMAP  500 //位图信息有误
#define ERRORW      304  //写不成功,长度不一致
#define NOFILE      400   //请求文件不存在
#define NOBLOCK     401  // 请求越界
#define MODSIZE　　 405 //固定大小失败
#define OPENERROR   404  //创建固定大小文件失败，本地存在也是返回失败，防止覆盖
#define OPENOK      403  //创建固定大小文件成功
#define FILEEXITS   408  //文件已经存在，防止覆盖创建
#define MAXCLIENT   500  // 请求超过上限
#define ERRORDATA   600  //接收错误
#define ERRORREAD   700  //读取文件错误
#define MALLOCERROR 800  // 动态空间分配错误
#define AIOERROR    900  //异步读错误
#define HASHERROR   901  //hash文件有错误
//#define INFOCLIENT  103 // 用户告知用户开辟多少空间来接收数据
#define ERRORALIGN  301    //写协议未按服务端的buffer对齐
#define ERRORSIZE   302  //写文件太大
#define ERRORWRITE  303  //服务器写错误
#define ERRORSPACE  402  //服务器空间分配错误，malloc/new 空间为NULL
#endif