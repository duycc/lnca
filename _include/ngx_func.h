// 一些函数声明
#ifndef __NGX_FUNC_H__
#define __NGX_FUNC_H__

/*------------------------字符串处理-------------------------*/

/**
 * @brief 去掉string右空格
 *
 * @param string
 */
void Rtrim(char *string);

/**
 * @brief 去掉string左空格
 *
 * @param string
 */
void Ltrim(char *string);

/*-----------------------设置进程标题------------------------*/

/**
 * @brief 移动环境变量地址
 */
void ngx_init_setproctitle();

/**
 * @brief 设置进程标题
 *
 * @param title
 */
void ngx_setproctitle(const char *title);

/*-----------------------日志打印输出------------------------*/

void ngx_log_init();
void ngx_log_stderr(int err, const char *fmt, ...);
void ngx_log_error_core(int level, int err, const char *fmt, ...);
u_char *ngx_log_errno(u_char *buf, u_char *last, int err);
u_char *ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...);
u_char *ngx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args);

/*-----------------------信号进程处理------------------------*/

int ngx_init_signals();
void ngx_master_process_cycle();
int ngx_daemon();

#endif
