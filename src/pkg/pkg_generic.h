#ifndef __PKG_GENERIC_H
#define __PKG_GENERIC_H

#include <stdlib.h>

int
handle_jre(const char *buf, size_t buf_len, const char *dest_path);

int
handle_jrepath(const char *buf, size_t buf_len, const char *dest_path);

int
handle_runner(const char *buf, size_t buf_len, const char *dest_path);

int
handle_all(
        const char *jre_buf, size_t jre_buf_len,
        const char *jrepath_buf, size_t jrepath_buf_len,
        const char *runner_buf, size_t runner_buf_len,
        const char *dest_path);

#endif
