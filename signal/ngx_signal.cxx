#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

/**
 * @brief 信号信息结构体
 */
typedef struct {
    int signo;            // 信号编号
    const char *signame;  // 信号名称

    void (*handler)(int signo, siginfo_t *siginfo,
                    void *ucontext);  // 信号处理函数
} ngx_signal_t;

static void ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext);
static void ngx_process_get_status(void);

ngx_signal_t signals[] = {
    // signo      signame             handler
    {SIGHUP,    "SIGHUP",           ngx_signal_handler},    // 终端断开信号
    {SIGINT,    "SIGINT",           ngx_signal_handler},    // 标识2
    {SIGTERM,   "SIGTERM",          ngx_signal_handler},    // 标识15
    {SIGCHLD,   "SIGCHLD",          ngx_signal_handler},    // 子进程退出时，父进程会收到这个信号--标识17
    {SIGQUIT,   "SIGQUIT",          ngx_signal_handler},    // 标识3
    {SIGIO,     "SIGIO",            ngx_signal_handler},    // 指示一个异步I/O事件【通用异步I/O信号】
    {SIGSYS,    "SIGSYS, SIG_IGN",  NULL},                  // 忽略这个信号

    {0,         NULL,               NULL}                   // 信号对应的数字至少是1，所以可以用0作为一个特殊标记
};

/**
 * @brief 信号初始化函数
 *
 * @return 0 成功，-1 失败
 */
int ngx_init_signals()
{
    ngx_signal_t *sig;
    struct sigaction sa;

    for (sig = signals; sig->signo != 0; sig++) {
        memset(&sa, 0, sizeof(struct sigaction));

        if (sig->handler) {
            sa.sa_sigaction = sig->handler;
            sa.sa_flags = SA_SIGINFO;
        }
        else {
            sa.sa_handler = SIG_IGN;  // 防止信号做默认处理（被系统kill）
        }                             // end if

        sigemptyset(&sa.sa_mask);

        if (sigaction(sig->signo, &sa, NULL) == -1) {
            ngx_log_error_core(NGX_LOG_EMERG, errno, "sigaction(%s) failed",
                               sig->signame);
            return -1;
        }
    }          // end for
    return 0;  // 成功
}

static void ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext)
{
    ngx_signal_t *sig;
    char *action;

    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    action = (char *)"";

    if (ngx_process == NGX_PROCESS_MASTER) {
        switch (signo) {
            case SIGCHLD:
                ngx_reap = 1;
                break;

            default:
                break;
        }
    }
    else if (ngx_process == NGX_PROCESS_WORKER) {
    }
    else {
    }

    // si_pid = sending process ID【发送该信号的进程id】
    if (siginfo && siginfo->si_pid) {
        ngx_log_error_core(NGX_LOG_NOTICE, 0,
                           "signal %d (%s) received from %P%s", signo,
                           sig->signame, siginfo->si_pid, action);
    }
    else {
        ngx_log_error_core(NGX_LOG_NOTICE, 0, "signal %d (%s) received %s",
                           signo, sig->signame, action);
    }

    return;
}

/**
 * @brief 获取子进程状态，防止出现单独kill子进程时子进程成为僵尸进程
 */
static void ngx_process_get_status(void)
{
    pid_t pid;
    int status;
    int err;
    int one = 0;

    for (;;) {
        pid = waitpid(-1, &status, WNOHANG);

        if (pid == 0) {
            return;
        }
        if (pid == -1) {
            err = errno;
            if (err == EINTR) {
                continue;
            }

            if (err == ECHILD && one) {
                return;
            }

            if (err == ECHILD) {
                ngx_log_error_core(NGX_LOG_INFO, err, "waitpid() failed!");
                return;
            }
            ngx_log_error_core(NGX_LOG_ALERT, err, "waitpid() failed!");
            return;
        }

        one = 1;               // 标记waitpid()返回了正常的返回值
        if (WTERMSIG(status))  // 获取使子进程终止的信号编号
        {
            ngx_log_error_core(NGX_LOG_ALERT, 0,
                               "pid = %P exited on signal %d!", pid,
                               WTERMSIG(status));
        }
        else {
            ngx_log_error_core(
                NGX_LOG_NOTICE, 0, "pid = %P exited with code %d!", pid,
                WEXITSTATUS(
                    status));  // WEXITSTATUS()获取子进程传递给exit或者_exit参数的低八位
        }
    }
    return;
}
