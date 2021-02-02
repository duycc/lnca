
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ngx_c_conf.h"
#include "ngx_func.h"
#include "ngx_macro.h"

/**
 * @brief 守护进程初始化
 *
 * @return -1 失败，0 子进程， 1 父进程
 */
int ngx_daemon()
{
    switch (fork()) {
        case -1:
            ngx_log_error_core(NGX_LOG_EMERG, errno,
                               "ngx_daemon()中fork()失败!");
            return -1;
        case 0:
            break;
        default:
            return 1;
    }  // end switch

    // 子进程继续执行
    ngx_parent = ngx_pid;
    ngx_pid = getpid();

    if (setsid() == -1) {
        ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中setsid()失败!");
        return -1;
    }

    umask(0);

    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        ngx_log_error_core(NGX_LOG_EMERG, errno,
                           "ngx_daemon()中open(\"/dev/null\")失败!");
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) {
        ngx_log_error_core(NGX_LOG_EMERG, errno,
                           "ngx_daemon()中dup2(STDIN)失败!");
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {
        ngx_log_error_core(NGX_LOG_EMERG, errno,
                           "ngx_daemon()中dup2(STDOUT)失败!");
        return -1;
    }
    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            ngx_log_error_core(NGX_LOG_EMERG, errno,
                               "ngx_daemon()中close(fd)失败!");
            return -1;
        }
    }
    return 0;
}

