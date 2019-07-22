#ifndef __UTIL_H
#define __UTIL_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

// log =========================================================================

#define log_i(...) \
    printf("info:  %s: %s (%s:%d)\n", __FUNCTION__, __VA_ARGS__, __FILE__, __LINE__)

#define log_e(...) \
    fprintf(stderr, "error: %s: %s (%s:%d)\n", __FUNCTION__, __VA_ARGS__, __FILE__, __LINE__)

#define log_if(fmt, ...) \
    printf("info:  %s: " fmt " (%s:%d)\n", __FUNCTION__, __VA_ARGS__, __FILE__, __LINE__)

#define log_ef(fmt, ...) \
    fprintf(stderr, "error: %s: " fmt " (%s:%d)\n", __FUNCTION__, __VA_ARGS__, __FILE__, __LINE__)

// log ^========================================================================

// str =========================================================================

char *
repeat_str(const char *str, size_t n);

#ifdef _WIN32

char *strndup(const char *str, size_t len);

int vasprintf(char **strp, const char *fmt, va_list ap);

int asprintf(char **strp, const char *fmt, ...);

#endif


// str ^========================================================================

// fs ==========================================================================

char *
join_path_(char *nop, ...);

#define join_path(...) \
    join_path_(NULL, __VA_ARGS__)

const char *
get_home_path();

bool
check_file_exist(const char *path);

int
mkdirp(const char *path);

int
mkdirp_for_file(const char *file_path);

// fs ^=========================================================================

// java fs =====================================================================

char *
make_abs_java_path(const char *root_path, const char *buf, size_t buf_len);

// java fs ^====================================================================

// tar =========================================================================

int
untar(const char *buf, size_t buf_len, const char *dest_path);

// tar ^========================================================================

// gz ==========================================================================

int
ungz(const char *src, size_t src_len, char **dst, size_t *dst_len);

// gz ^=========================================================================

#endif
