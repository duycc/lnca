
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ngx_c_conf.h"
#include "ngx_func.h"
#include "ngx_macro.h"

// 函数声明
static void ngx_start_worker_processes(int threadnums);
static int ngx_spawn_process(int threadnums, const char *pprocname);
static void ngx_worker_process_cycle(int inum, const char *pprocname);
static void ngx_worker_process_init(int inum);

// 变量声明
static u_char master_process[] = "master process";

/**
 * @brief 创建 worker 子进程
 */
void ngx_master_process_cycle()
{
    sigset_t set;  // 信号集

    sigemptyset(&set);  // 清空信号集

    sigaddset(&set, SIGCHLD);   // 子进程状态改变
    sigaddset(&set, SIGALRM);   // 定时器超时
    sigaddset(&set, SIGIO);     // 异步I/O
    sigaddset(&set, SIGINT);    // 终端中断符
    sigaddset(&set, SIGHUP);    // 连接断开
    sigaddset(&set, SIGUSR1);   // 用户定义信号
    sigaddset(&set, SIGUSR2);   // 用户定义信号
    sigaddset(&set, SIGWINCH);  // 终端窗口大小改变
    sigaddset(&set, SIGTERM);   // 终止
    sigaddset(&set, SIGQUIT);   // 终端退出符

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        ngx_log_error_core(NGX_LOG_ALERT, errno,
                           "ngx_master_process_cycle()中sigprocmask()失败!");
    }
    // 即便sigprocmask失败，程序流程继续往下走

    size_t size;
    int i;
    size = sizeof(master_process);
    size += g_argvneedmem;
    if (size < 1000) {
        char title[1000] = {0};
        strcpy(title, (const char *)master_process);
        strcat(title, " ");
        for (i = 0; i < g_os_argc; i++) {
            strcat(title, g_os_argv[i]);
            strcat(title, " ");
        }
        ngx_setproctitle(title);
        ngx_log_error_core(NGX_LOG_NOTICE, 0, "%s %P 启动并开始运行......!",
                           title, ngx_pid);
    }

    CConfig *p_config = CConfig::GetInstance();
    int workprocess = p_config->GetIntDefault("WorkerProcesses", 1);
    ngx_start_worker_processes(workprocess);

    // 创建子进程后，父进程的执行流程会返回到这里，子进程不会走进来
    sigemptyset(&set);

    for (;;) {
        sigsuspend(
            &set);  //阻塞等待信号，进程挂起不占用cpu时间，收到信号才被唤醒，此时master进程完全靠信号驱动干活

        sleep(1);
    }
    return;
}

/**
 * @brief 创建指定数量子进程
 *
 * @param threadnums
 */
static void ngx_start_worker_processes(int threadnums)
{
    int i;
    for (i = 0; i < threadnums; i++) {
        ngx_spawn_process(i, "worker process");
    }  // end for
    return;
}

/**
 * @brief 创建一个子进程
 *
 * @param inum 进程编号
 * @param pprocname 进程名
 *
 * @return
 */
static int ngx_spawn_process(int inum, const char *pprocname)
{
    pid_t pid;

    pid = fork();
    switch (pid) {
        case -1:  // 产生子进程失败
            ngx_log_error_core(NGX_LOG_ALERT, errno,
                               "ngx_spawn_process()fork()产生子进程num=%d,"
                               "procname=\"%s\"失败!",
                               inum, pprocname);
            return -1;

        case 0:  // 子进程分支
            ngx_parent = ngx_pid;
            ngx_pid = getpid();
            ngx_worker_process_cycle(inum, pprocname);
            break;

        default:  // 父进程分支，直接break，流程往switch之后走
            break;
    }  // end switch

    return pid;
}

/**
 * @brief 子进程功能函数
 *
 * @param inum
 * @param pprocname
 */
static void ngx_worker_process_cycle(int inum, const char *pprocname)
{
    ngx_process = NGX_PROCESS_WORKER;

    ngx_worker_process_init(inum);
    ngx_setproctitle(pprocname);
    ngx_log_error_core(NGX_LOG_NOTICE, 0, "%s %P 启动并开始运行......!",
                       pprocname, ngx_pid);

    for (;;) {
        sleep(1);  // 休息1秒

    }  // end for(;;)
    return;
}

static void ngx_worker_process_init(int inum)
{
    sigset_t set;

    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        ngx_log_error_core(NGX_LOG_ALERT, errno,
                           "ngx_worker_process_init()中sigprocmask()失败!");
    }

    //....
    return;
}
