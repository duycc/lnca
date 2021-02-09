#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "ngx_c_conf.h"
#include "ngx_c_socket.h"
#include "ngx_func.h"
#include "ngx_macro.h"

//构造函数
CSocekt::CSocekt()
{
    m_ListenPortCount = 1;  //监听一个端口
}

//释放函数
CSocekt::~CSocekt()
{
    //释放必须的内存
    std::vector<lpngx_listening_t>::iterator pos;
    for (pos = m_ListenSocketList.begin(); pos != m_ListenSocketList.end();
         ++pos) {
        delete (*pos);
    }  // end for
    m_ListenSocketList.clear();
}

/**
 * @brief 初始化listenfd
 *
 * @return true 成功, false 失败
 */
bool CSocekt::Initialize()
{
    bool reco = ngx_open_listening_sockets();
    return reco;
}

/**
 * @brief 创建监听端口
 *
 * @return true 成功
 */
bool CSocekt::ngx_open_listening_sockets()
{
    CConfig *p_config = CConfig::GetInstance();
    m_ListenPortCount = p_config->GetIntDefault(
        "ListenPortCount", m_ListenPortCount);  //取得要监听的端口数量

    int isock;                     // socket
    struct sockaddr_in serv_addr;  //服务器的地址结构体
    int iport;                     //端口
    char strinfo[100];             //临时字符串

    //初始化相关
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    for (int i = 0; i < m_ListenPortCount; i++) {
        isock = socket(AF_INET, SOCK_STREAM, 0);
        if (isock == -1) {
            ngx_log_stderr(errno, "CSocekt::Initialize()中socket()失败,i=%d.",
                           i);
            return false;
        }

        int reuseaddr = 1;
        if (setsockopt(isock, SOL_SOCKET, SO_REUSEADDR,
                       (const void *)&reuseaddr, sizeof(reuseaddr)) == -1) {
            ngx_log_stderr(
                errno,
                "CSocekt::Initialize()中setsockopt(SO_REUSEADDR)失败,i=%d.", i);
            close(isock);
            return false;
        }
        //设置该socket为非阻塞
        if (setnonblocking(isock) == false) {
            ngx_log_stderr(
                errno, "CSocekt::Initialize()中setnonblocking()失败,i=%d.", i);
            close(isock);
            return false;
        }

        //设置本服务器要监听的地址和端口
        strinfo[0] = 0;
        sprintf(strinfo, "ListenPort%d", i);
        iport = p_config->GetIntDefault(strinfo, 10000);
        serv_addr.sin_port = htons((in_port_t)iport);

        //绑定服务器地址结构体
        if (bind(isock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) ==
            -1) {
            ngx_log_stderr(errno, "CSocekt::Initialize()中bind()失败,i=%d.", i);
            close(isock);
            return false;
        }

        //开始监听
        if (listen(isock, NGX_LISTEN_BACKLOG) == -1) {
            ngx_log_stderr(errno, "CSocekt::Initialize()中listen()失败,i=%d.",
                           i);
            close(isock);
            return false;
        }

        lpngx_listening_t p_listensocketitem = new ngx_listening_t;
        memset(p_listensocketitem, 0, sizeof(ngx_listening_t));
        p_listensocketitem->port = iport;
        p_listensocketitem->fd = isock;
        ngx_log_error_core(NGX_LOG_INFO, 0, "监听%d端口成功!", iport);
        m_ListenSocketList.push_back(p_listensocketitem);
    }  // end for(int i = 0; i < m_ListenPortCount; i++)
    return true;
}

bool CSocekt::setnonblocking(int sockfd)
{
    int nb = 1;
    // FIONBIO：设置/清除非阻塞I/O标记：0：清除，1：设置
    if (ioctl(sockfd, FIONBIO, &nb) == -1) {
        return false;
    }
    return true;
}

void CSocekt::ngx_close_listening_sockets()
{
    for (int i = 0; i < m_ListenPortCount; i++) {
        close(m_ListenSocketList[i]->fd);
        ngx_log_error_core(NGX_LOG_INFO, 0, "关闭监听端口%d!",
                           m_ListenSocketList[i]->port);
    }  // end for(int i = 0; i < m_ListenPortCount; i++)
    return;
}
