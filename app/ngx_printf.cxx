
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

static u_char *ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64,
                               u_char zero, uintptr_t hexadecimal,
                               uintptr_t width);

u_char *ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...)
{
    va_list args;
    u_char *p;

    va_start(args, fmt);
    p = ngx_vslprintf(buf, last, fmt, args);
    va_end(args);
    return p;
}

u_char *ngx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args)
{
    u_char zero;

    /*
    #ifdef _WIN64
        typedef unsigned __int64  uintptr_t;
    #else
        typedef unsigned int uintptr_t;
    #endif
    */
    uintptr_t width, sign, hex, frac_width, scale, n;

    int64_t i64;    // 保存 %d 对应的可变参
    uint64_t ui64;  // 保存 %ud 对应的可变参
    u_char *p;      // 保存 %s 对应的可变参
    double f;       // 保存 %f 对应的可变参
    uint64_t frac;  // %f 可变参数,根据%.2f等，取得小数部分的2位后的内容；

    // 每次处理一个字符
    while (*fmt && buf < last) {
        // %开头需要被可变参数取代
        if (*fmt == '%') {
            zero = (u_char)((*++fmt == '0') ? '0' : ' ');  // 是否 0 填充

            width = 0;
            sign = 1;
            hex = 0;
            frac_width = 0;  // 小数点后位数字
            i64 = 0;
            ui64 = 0;

            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt++ - '0');
            }

            for (;;) {
                switch (*fmt) {
                    case 'u':
                        sign = 0;
                        fmt++;
                        continue;

                    case 'X':
                        hex = 2;  // 标记以大写字母显示十六进制中的A-F
                        sign = 0;
                        fmt++;
                        continue;
                    case 'x':
                        hex = 1;  // 标记以小写字母显示十六进制中的a-f
                        sign = 0;
                        fmt++;
                        continue;

                    case '.':
                        fmt++;
                        while (*fmt >= '0' && *fmt <= '9') {
                            frac_width = frac_width * 10 + (*fmt++ - '0');
                        }
                        break;

                    default:
                        break;
                }
                break;
            }

            switch (*fmt) {
                case '%':  // %%
                    *buf++ = '%';
                    fmt++;
                    continue;

                case 'd':
                    if (sign) {
                        i64 = (int64_t)va_arg(args, int);
                    }
                    else {
                        ui64 = (uint64_t)va_arg(args, u_int);
                    }
                    break;

                case 's':
                    p = va_arg(args, u_char *);

                    while (*p && buf < last) {
                        *buf++ = *p++;
                    }

                    fmt++;
                    continue;

                case 'P':
                    i64 = (int64_t)va_arg(args, pid_t);
                    sign = 1;
                    break;

                case 'f':
                    f = va_arg(args, double);
                    if (f < 0) {
                        *buf++ = '-';
                        f = -f;
                    }

                    ui64 = (int64_t)f;
                    frac = 0;

                    if (frac_width) {
                        scale = 1;  // 缩放从1开始
                        for (n = frac_width; n; n--) {
                            scale *= 10;
                        }

                        frac = (uint64_t)((f - (double)ui64) * scale + 0.5);

                        if (frac == scale) {
                            ui64++;
                            frac = 0;
                        }
                    }
                    buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width);

                    if (frac_width) {
                        if (buf < last) {
                            *buf++ = '.';
                        }
                        buf = ngx_sprintf_num(buf, last, frac, '0', 0,
                                              frac_width);
                    }
                    fmt++;
                    continue;

                default:
                    *buf++ = *fmt++;
                    continue;
            }

            if (sign) {
                if (i64 < 0) {
                    *buf++ = '-';
                    ui64 = (uint64_t)-i64;
                }
                else {
                    ui64 = (uint64_t)i64;
                }
            }
            buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width);
            fmt++;
        }
        else {
            *buf++ = *fmt++;
        }  // end if (*fmt == '%')
    }      // end while (*fmt && buf < last)

    return buf;
}

static u_char *ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64,
                               u_char zero, uintptr_t hexadecimal,
                               uintptr_t width)
{
    u_char *p, temp[NGX_INT64_LEN + 1];
    size_t len;
    uint32_t ui32;

    static u_char hex[] = "0123456789abcdef";
    static u_char HEX[] = "0123456789ABCDEF";

    p = temp + NGX_INT64_LEN;

    if (hexadecimal == 0) {
        if (ui64 <= (uint64_t)NGX_MAX_UINT32_VALUE) {
            ui32 = (uint32_t)ui64;
            do

            {
                *--p = (u_char)(ui32 % 10 + '0');
            } while (ui32 /= 10);
        }
        else {
            do {
                *--p = (u_char)(ui64 % 10 + '0');
            } while (ui64 /= 10);
        }
    }
    else if (hexadecimal == 1) {
        do {
            *--p = hex[(uint32_t)(ui64 & 0xf)];
        } while (ui64 >>= 4);
    }
    else {
        do {
            *--p = HEX[(uint32_t)(ui64 & 0xf)];
        } while (ui64 >>= 4);
    }

    len = (temp + NGX_INT64_LEN) - p;

    while (len++ < width && buf < last) {
        *buf++ = zero;
    }

    len = (temp + NGX_INT64_LEN) - p;
    if ((buf + len) >= last) {
        len = last - buf;
    }

    return ngx_cpymem(buf, p, len);
}

