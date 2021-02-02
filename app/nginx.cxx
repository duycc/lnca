// 主程序入口

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ngx_c_conf.h"  // 和配置文件处理相关的类
#include "ngx_func.h"    // 各种函数声明
#include "ngx_macro.h"   // 各种宏定义

// 程序退出释放资源
static void freeresource();

// 设置标题有关的全局量
size_t g_argvneedmem = 0;  // argv参数所需要的内存大小
size_t g_envneedmem = 0;   // 环境变量所占内存大小
int g_os_argc;             // 参数个数
char **g_os_argv;          // 原始命令行参数数组
char *gp_envmem = NULL;  // 自定义环境变量内存地址，后续需要移动环境变量内存地址
int g_daemonized = 0;  // 守护进程标记 1 守护运行，0 非守护进程方式运行

// 进程相关全局量
pid_t ngx_pid;          // 当前进程的pid
pid_t ngx_parent;       // 父进程的pid
int ngx_process;        // 进程类型 master worker
sig_atomic_t ngx_reap;  // 标记子进程状态

int main(int argc, char *const *argv)
{
    int exitcode = 0;
    int i;

    ngx_pid = getpid();      // 进程pid
    ngx_parent = getppid();  // 父进程id

    // 统计argv所占的内存
    g_argvneedmem = 0;
    for (i = 0; i < argc; i++) {
        g_argvneedmem += strlen(argv[i]) + 1;
    }
    // 统计环境变量所占的内存
    for (i = 0; environ[i]; i++) {
        g_envneedmem += strlen(environ[i]) + 1;
    }

    g_os_argc = argc;           // 保存参数个数
    g_os_argv = (char **)argv;  // 保存参数指针

    ngx_log.fd = -1;                   // -1 表示日志文件尚未打开
    ngx_process = NGX_PROCESS_MASTER;  // 标记本进程是master进程
    ngx_reap = 0;                      // 标记子进程没有发生变化

    CConfig *p_config = CConfig::GetInstance();
    if (p_config->Load("nginx.conf") == false) {
        ngx_log_init();
        ngx_log_stderr(0, "配置文件[%s]载入失败，退出!", "nginx.conf");

        exitcode = 2;  // 2 标记找不到文件
        goto lblexit;
    }

    ngx_log_init();

    if (ngx_init_signals() != 0) {
        exitcode = 1;
        goto lblexit;
    }

    ngx_init_setproctitle();

    if (p_config->GetIntDefault("Daemon", 0) == 1) {
        int cdaemonresult = ngx_daemon();
        if (cdaemonresult == -1) {
            exitcode = 1;
            goto lblexit;
        }
        if (cdaemonresult == 1) {
            freeresource();
            exitcode = 0;
            return exitcode;
        }

        g_daemonized = 1;  // 0 未启用守护进程模式，1 启用
    }

    // start work...
    ngx_master_process_cycle();

lblexit:
    ngx_log_stderr(0, "程序退出，再见了!");
    freeresource();
    return exitcode;
}

void freeresource()
{
    if (gp_envmem) {
        delete[] gp_envmem;
        gp_envmem = NULL;
    }

    if (ngx_log.fd != STDERR_FILENO && ngx_log.fd != -1) {
        close(ngx_log.fd);
        ngx_log.fd = -1;  // 标记，防止被再次 close()
    }
}
