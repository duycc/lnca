
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "ngx_c_conf.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

//错误等级，和ngx_macro.h里定义的日志等级宏对应
static u_char err_levels[][20] = {
    {"stderr"},  // 0：控制台错误
    {"emerg"},   // 1：紧急
    {"alert"},   // 2：警戒
    {"crit"},    // 3：严重
    {"error"},   // 4：错误
    {"warn"},    // 5：警告
    {"notice"},  // 6：注意
    {"info"},    // 7：信息
    {"debug"}    // 8：调试
};
ngx_log_t ngx_log;

/* test ngx_log_stderr()

ngx_log_stderr(0, "invalid option: \"%s\"", argv[0]);  //nginx: invalid option: "./nginx"
ngx_log_stderr(0, "invalid option: %10d", 21); //nginx: invalid option:         21  ---21前面有8个空格
ngx_log_stderr(0, "invalid option: %.6f", 21.378);     //nginx: invalid option: 21.378000 ---%.这种只跟f配合有效，往末尾填充0 
ngx_log_stderr(0, "invalid option: %.6f", 12.999);     //nginx: invalid option: 12.999000
ngx_log_stderr(0, "invalid option: %.2f", 12.999);     //nginx: invalid option: 13.00
ngx_log_stderr(0, "invalid option: %xd", 1678);        //nginx: invalid option: 68E
ngx_log_stderr(0, "invalid option: %Xd", 1678);        //nginx: invalid option: 68E
ngx_log_stderr(15, "invalid option: %s , %d", "testInfo",326);        //nginx: invalid option: testInfo , 326
ngx_log_stderr(0, "invalid option: %d", 1678);
*/

void ngx_log_stderr(int err, const char *fmt, ...)
{
    va_list args;
    u_char errstr[NGX_MAX_ERROR_STR + 1];
    u_char *p, *last;

    memset(errstr, 0, sizeof(errstr));

    last = errstr + NGX_MAX_ERROR_STR;

    p = ngx_cpymem(errstr, "nginx: ", 7);

    va_start(args, fmt);                    // 使args指向起始的参数
    p = ngx_vslprintf(p, last, fmt, args);  // 组合出这个字符串保存在errstr里
    va_end(args);                           //释放args

    if (err) {  // 如果错误代码不是0，表示有错误发生，错误代码和错误信息也要显示出来
        p = ngx_log_errno(p, last, err);
    }

    // 末尾插入'\n'
    if (p >= (last - 1)) {
        p = (last - 1) - 1;
    }
    *p++ = '\n';

    // 往标准错误 屏幕 输出信息
    write(STDERR_FILENO, errstr, p - errstr);

    return;
}

// err：错误编号，取得这个错误编号对应的错误字符串，保存到buf中
u_char *ngx_log_errno(u_char *buf, u_char *last, int err)
{
    char *perrorinfo = strerror(err);
    size_t len = strlen(perrorinfo);

    char leftstr[10] = {0};
    sprintf(leftstr, " (%d: ", err);
    size_t leftlen = strlen(leftstr);

    char rightstr[] = ") ";
    size_t rightlen = strlen(rightstr);

    size_t extralen = leftlen + rightlen;
    if ((buf + len + extralen) < last) {
        buf = ngx_cpymem(buf, leftstr, leftlen);
        buf = ngx_cpymem(buf, perrorinfo, len);
        buf = ngx_cpymem(buf, rightstr, rightlen);
    }
    return buf;
}

void ngx_log_error_core(int level, int err, const char *fmt, ...)
{
    u_char *last;
    u_char errstr[NGX_MAX_ERROR_STR + 1];

    memset(errstr, 0, sizeof(errstr));
    last = errstr + NGX_MAX_ERROR_STR;

    struct timeval tv;
    struct tm tm;
    time_t sec;
    u_char *p;
    va_list args;

    memset(&tv, 0, sizeof(struct timeval));
    memset(&tm, 0, sizeof(struct tm));

    gettimeofday(&tv, NULL);

    sec = tv.tv_sec;  // 秒
    localtime_r(&sec, &tm);  // 把参数1的 time_t 转换为本地时间，保存到参数2中
    tm.tm_mon++;         // 月份要调整
    tm.tm_year += 1900;  // 年份要调整

    u_char strcurrtime[40] = {
        0};  // 先组合出一个当前时间字符串，格式形如：2021/01/28 19:57:11
    ngx_slprintf(strcurrtime, (u_char *)-1,
                 "%4d/%02d/%02d %02d:%02d:%02d",  // 格式是 年/月/日 时:分:秒
                 tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min,
                 tm.tm_sec);
    p = ngx_cpymem(
        errstr, strcurrtime,
        strlen((const char *)strcurrtime));  // 日期      2021/01/28 20:26:07
    p = ngx_slprintf(
        p, last, " [%s] ",
        err_levels[level]);  // 日志级别  2021/01/28 20:26:07 [crit]
    p = ngx_slprintf(
        p, last, "%P: ", ngx_pid);  // 进程id 2021/01/28 20:50:15 [crit] 2037

    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);

    if (err) {
        p = ngx_log_errno(p, last, err);
    }
    if (p >= (last - 1)) {
        p = (last - 1) - 1;
    }
    *p++ = '\n';  // 添加换行符

    ssize_t n;
    while (1) {
        if (level > ngx_log.log_level) {
            break;
        }

        n = write(ngx_log.fd, errstr, p - errstr);
        if (n == -1) {
            if (errno == ENOSPC) {
                // 垃圾电脑磁盘容量不足
            }
            else {
                if (ngx_log.fd != STDERR_FILENO) {
                    n = write(STDERR_FILENO, errstr, p - errstr);
                }
            }
        }
        break;
    }  // end while
    return;
}

// 打开并初始化日志文件
void ngx_log_init()
{
    u_char *plogname = NULL;
    size_t nlen;

    CConfig *p_config = CConfig::GetInstance();
    plogname = (u_char *)p_config->GetString("Log");
    if (plogname == NULL) {
        plogname = (u_char *)
            NGX_ERROR_LOG_PATH;  //"logs/error.log" ,logs目录需要提前建立
    }
    ngx_log.log_level = p_config->GetIntDefault("LogLevel", NGX_LOG_NOTICE);

    ngx_log.fd =
        open((const char *)plogname, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (ngx_log.fd == -1) {
        ngx_log_stderr(
            errno,
            "[alert] could not open error log file: open() \"%s\" failed",
            plogname);
        ngx_log.fd = STDERR_FILENO;
    }
    return;
}
