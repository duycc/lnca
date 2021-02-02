
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ngx_global.h"

/**
 * @brief 环境变量移动
 */
void ngx_init_setproctitle()
{
    gp_envmem = new char[g_envneedmem];
    memset(gp_envmem, 0, g_envneedmem);

    char *ptmp = gp_envmem;
    for (int i = 0; environ[i]; i++) {
        size_t size = strlen(environ[i]) + 1;
        strcpy(ptmp, environ[i]);  // 把原环境变量内容拷贝到新内存
        environ[i] = ptmp;         // 让新环境变量指向这段新内存
        ptmp += size;
    }
    return;
}

/**
 * @brief 设置进程名
 *
 * @param title
 */
void ngx_setproctitle(const char *title)
{
    size_t ititlelen = strlen(title);

    size_t esy = g_argvneedmem + g_envneedmem;
    if (esy <= ititlelen) {
        return;
    }

    g_os_argv[1] = NULL;

    char *ptmp = g_os_argv[0];
    strcpy(ptmp, title);
    ptmp += ititlelen;

    size_t cha = esy - ititlelen;
    memset(ptmp, 0, cha);
    return;
}
