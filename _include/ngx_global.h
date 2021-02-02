
#ifndef __NGX_GBLDEF_H__
#define __NGX_GBLDEF_H__

#include <signal.h>

typedef struct {
    char ItemName[50];
    char ItemContent[500];
} CConfItem, *LPCConfItem;

typedef struct {
    int log_level;  // 日志级别
    int fd;         // 日志文件描述符

} ngx_log_t;

// 外部全局量声明
extern size_t g_argvneedmem;
extern size_t g_envneedmem;
extern int g_os_argc;
extern char** g_os_argv;
extern char* gp_envmem;
extern int g_daemonized;

extern pid_t ngx_pid;
extern pid_t ngx_parent;
extern ngx_log_t ngx_log;
extern int ngx_process;
extern sig_atomic_t ngx_reap;

#endif
