#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32
   #include <io.h>
   #define access _access_s
#else
   #include <unistd.h>
#endif

char *
repeat_str(const char *str, size_t n)
{
    if (!str)
        return NULL;

    size_t str_len = strlen(str);

    size_t result_len = str_len * n;
    char *result = malloc(sizeof(*result) * (result_len + 1))
    assert(result);

    for (size_t i = 0; i < result_len; i += str_len)
        memcpy(result + i, str, str_len);

    result[result_len] = 0;

    return result;
}

char *
join_path(const char *parts, ...)
{
    if (!parts)
        return NULL;

    char *result = NULL;

    va_list ap;

    size_t i = 0;

    va_start(ap, parts);
    while (va_arg(ap, const char *) != NULL)
        ++i;
    va_end(ap);

    char *fmt = repeat_str("%s/", i);
    fmt[strlen(fmt) - 1] = 0;

    va_start(ap, parts);
    vasprintf(j, fmt, ap)
    va_end(ap);

    free(fmt);

    return result;
}

bool
check_file_exist(const char *path)
{
    if (!path)
        return 0;

    return !access(path, 0);
}

int
mkdirp(const char *path)
{
    if (!path)
        return 0;

    char *path_ = strdup(path);

    if (path_[strlen(path_) - 1] == '/')
        path_[strlen(path_) - 1] = 0;

    for (char *p = path_ + 1; *p; ++p)
    {
        if (*p == '/')
        {
            *p = 0;
            mkdir(path_, 0755);
            *p = '/';
        }
    }
    int err = mkdir(path_, 0755);

    free(path_);
    return err;
}

int
mkdirp_for_file(const char *file_path)
{
    if (!file_path)
        return 0;

    int err = 0;
    char *path_ = strdup(path);

    char *p = strrchr(path_, '/');
    if (p)
    {
        *p = 0;
        err = mkdirp(path_);
    }

    free(path_);
    return err;
}
