
#ifndef __NGX_MACRO_H__
#define __NGX_MACRO_H__

#define NGX_MAX_ERROR_STR 2048  // 显示的错误信息最大数组长度

// ngx_cpymem()返回的是目标拷贝数据后的终点位置，连续复制多段数据时方便
#define ngx_cpymem(dst, src, n) (((u_char *)memcpy(dst, src, n)) + (n))
#define ngx_min(val1, val2) ((val1 > val2) ? (val2) : (val1))

#define NGX_MAX_UINT32_VALUE (uint32_t)0xffffffff
#define NGX_INT64_LEN (sizeof("-9223372036854775808") - 1)

#define NGX_LOG_STDERR 0  //控制台错误【stderr】直接输出到控制台
#define NGX_LOG_EMERG 1   //紧急 【emerg】
#define NGX_LOG_ALERT 2   //警戒 【alert】
#define NGX_LOG_CRIT 3    //严重 【crit】
#define NGX_LOG_ERR 4     //错误 【error】
#define NGX_LOG_WARN 5    //警告 【warn】
#define NGX_LOG_NOTICE 6  //注意 【notice】
#define NGX_LOG_INFO 7    //信息 【info】
#define NGX_LOG_DEBUG 8   //调试 【debug】

#define NGX_ERROR_LOG_PATH "error.log"

#define NGX_PROCESS_MASTER 0  // master进程，管理进程
#define NGX_PROCESS_WORKER 1  // worker进程，工作进程

#endif
