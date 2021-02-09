
#ifndef __NGX_SOCKET_H__
#define __NGX_SOCKET_H__

#include <vector>

#define NGX_LISTEN_BACKLOG 511  // 已完成连接数目最大值

typedef struct ngx_listening_s {
    int port;  // 监听的端口号
    int fd;    // 套接字句柄socket
} ngx_listening_t, *lpngx_listening_t;

class CSocekt {
public:
    CSocekt();           //构造函数
    virtual ~CSocekt();  //释放函数

public:
    virtual bool Initialize();  //初始化函数

private:
    bool ngx_open_listening_sockets();   //监听必须的端口
    void ngx_close_listening_sockets();  //关闭监听套接字
    bool setnonblocking(int sockfd);     //设置非阻塞套接字

private:
    int m_ListenPortCount;  //所监听的端口数量
    std::vector<lpngx_listening_t> m_ListenSocketList;  //监听套接字队列
};

#endif
