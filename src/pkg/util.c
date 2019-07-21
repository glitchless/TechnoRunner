#include "util.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#ifdef _WIN32
    #include <io.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <errno.h>
    #include <pwd.h>
#endif

#include <zlib.h>

#include "paths.h"

// str =========================================================================

char *
repeat_str(const char *str, size_t n)
{
    if (!str)
        return NULL;

    size_t str_len = strlen(str);

    size_t result_len = str_len * n;
    char *result = malloc(sizeof(*result) * (result_len + 1));
    assert(result);

    for (size_t i = 0; i < result_len; i += str_len)
        memcpy(result + i, str, str_len);

    result[result_len] = 0;

    return result;
}

// str ^========================================================================

// fs ==========================================================================

char *
join_path_(char *nop, ...)
{
    char *result = NULL;
    va_list ap;

    size_t parts_count = 0;
    va_start(ap, nop);
    while (va_arg(ap, const char *) != NULL)
        ++parts_count;
    va_end(ap);

    char *fmt = repeat_str("%s/", parts_count);
    fmt[strlen(fmt) - 1] = 0;

    va_start(ap, nop);
    vasprintf(&result, fmt, ap);
    va_end(ap);

    free(fmt);

    return result;
}

const char *
get_home_path()
{
#ifdef _WIN32

#else
    const char *from_env = getenv("HOME");
    if (from_env)
        return from_env;

    struct passwd *pw = getpwuid(getuid());
    assert(pw);
    return pw->pw_dir;
#endif
}

bool
check_file_exist(const char *path)
{
    if (!path)
        return 0;

#ifdef _WIN32
    return !_access_s(path, 0);
#else
    return !access(path, 0);
#endif
}

int
mkdirp(const char *path)
{
    if (!path)
        return 0;

    int err = 0;
    char *path_ = strdup(path);

    if (path_[strlen(path_) - 1] == '/')
        path_[strlen(path_) - 1] = 0;

    for (char *p = path_ + 1; *p; ++p)
    {
        if (*p == '/')
        {
            *p = 0;
            err = mkdir(path_, 0755);
            if (err < 0 && errno != EEXIST)
                goto err;
            *p = '/';
        }
    }

    err = mkdir(path_, 0755);
    if (err < 0 && errno != EEXIST)
        goto err;

    goto out;

err:
    log_ef("can't create dir '%s': %d ('%s')", path_, errno, strerror(errno));

out:
    free(path_);
    return err < 0 && errno != EEXIST;
}

int
mkdirp_for_file(const char *file_path)
{
    if (!file_path)
        return 0;

    int err = 0;
    char *path = strdup(file_path);

    char *p = strrchr(path, '/');
    if (p)
    {
        *p = 0;
        err = mkdirp(path);
    }

    free(path);
    return err;
}

// fs ^=========================================================================

// java fs =====================================================================

char *
make_abs_java_path(const char *root_path, const char *buf, size_t buf_len)
{
    char *java_path = NULL;

    char *buf_dup = strndup(buf, buf_len);
    for (size_t i = 0; i < buf_len; ++i)
    {
        if (buf_dup[i] == '\n')
        {
            buf_dup[i] = '\0';
            break;
        }
    }

    java_path = join_path(root_path, JRE_DIRNAME, buf_dup, NULL);

    free(buf_dup);
    return java_path;
}

// java fs ^====================================================================

// tar =========================================================================

// It's modified https://github.com/libarchive/libarchive/blob/master/contrib/untar.c

/*
 * This file is in the public domain.  Use it as you see fit.
 */

/*
 * "untar" is an extremely simple tar extractor:
 *  * A single C source file, so it should be easy to compile
 *    and run on any system with a C compiler.
 *  * Extremely portable standard C.  The only non-ANSI function
 *    used is mkdir().
 *  * Reads basic ustar tar archives.
 *  * Does not require libarchive or any other special library.
 *
 * To compile: cc -o untar untar.c
 *
 * Usage:  untar <archive>
 *
 * In particular, this program should be sufficient to extract the
 * distribution for libarchive, allowing people to bootstrap
 * libarchive on systems that do not already have a tar program.
 *
 * To unpack libarchive-x.y.z.tar.gz:
 *    * gunzip libarchive-x.y.z.tar.gz
 *    * untar libarchive-x.y.z.tar
 *
 * Written by Tim Kientzle, March 2009.
 *
 * Released into the public domain.
 */

/* Parse an octal number, ignoring leading and trailing nonsense. */
static int
parseoct(const char *p, size_t n)
{
    int i = 0;

    while ((*p < '0' || *p > '7') && n > 0)
    {
        ++p;
        --n;
    }
    while (*p >= '0' && *p <= '7' && n > 0)
    {
        i *= 8;
        i += *p - '0';
        ++p;
        --n;
    }

    return i;
}

/* Returns true if this is 512 zero bytes. */
static bool
is_end_of_archive(const char *p)
{
    for (int n = 511; n >= 0; --n)
        if (p[n] != '\0')
            return 0;
    return 1;
}

/* Verify the tar checksum. */
static int
verify_checksum(const char *p)
{
    int n, u = 0;
    for (n = 0; n < 512; ++n) {
        if (n < 148 || n > 155)
            /* Standard tar checksum adds unsigned bytes. */
            u += ((unsigned char *)p)[n];
        else
            u += 0x20;
    }
    return (u == parseoct(p + 148, 8));
}

/* Extract a tar archive. */
int
untar(const char *buf, size_t buf_len, const char *dest_path)
{
    if (!buf || !dest_path)
        return 0;

    int err = 0;
    const char *buf_end = buf + buf_len;

    for (const char *p = buf; p < buf_end; p += 512)
    {
        if ((buf_end - p) < 512)
        {
            log_ef("bad tar part len: %ld < 512", buf_end - p);
            return 1;
        }
        if (is_end_of_archive(p))
        {
            goto ok;
        }
        if (!verify_checksum(p))
        {
            log_e("bad tar checksum");
            return 2;
        }

        FILE *file = NULL;
        char *file_path = NULL;
        size_t file_size = parseoct(p + 124, 12);
        switch (p[156])
        {
            case '1':
            case '2':
            case '3':
            case '4':
            case '6':
                break;
            case '5':
                file_path = join_path(dest_path, p, NULL);
                if (mkdirp(file_path))
                {
                    log_ef("can't create dir '%s'", p);
                    err = 3;
                    goto lend;
                }
                file_size = 0;
                break;
            default:
                file_path = join_path(dest_path, p, NULL);
                if (mkdirp_for_file(file_path))
                {
                    log_ef("can't create parent dir for '%s'", p);
                    err = 4;
                    goto lend;
                }
                file = fopen(file_path, "wb");
        }

        if (file_size)
        {
            if (!file)
            {
                log_ef("can't open file '%s'", file_path);
                err = 5;
                goto lend;
            }

            p += 512;
            if ((p + file_size) >= buf_end)
            {
                log_e("bad tar");
                err = 6;
                goto lend;
            }

            log_if("unpack '%s'", file_path);
            int r = fwrite(p, 1, file_size, file);

            if (r != file_size)
            {
                log_ef("can't write file '%s'", file_path);
                err = 7;
                goto lend;
            }

            p += file_size % 512 ? (file_size / 512 + 0) * 512 : file_size - 512;
        }

lend:
        if (file)
            fclose(file);

        free(file_path);

        if (err)
            return err;
    }

    log_e("bad tar");
    return 7;

ok:
    return 0;
}

// tar ^========================================================================

// gz ==========================================================================

#define UNGZ_CHUNK 16384

int
ungz(const char *src, size_t src_len, char **dst, size_t *dst_len)
{
    if (!src)
        return 0;

    int err = 0;

    *dst_len = 0;
    *dst = NULL;
    size_t dst_offset = 0;

    z_stream strm = {0};
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = strm.total_in = src_len;
    strm.next_in  = (char *) src;
    if (inflateInit2(&strm, 16 + MAX_WBITS))
    {
        log_e("bad zlib init");
        err = 1;
        goto out;
    }

    while (1)
    {
        *dst_len += UNGZ_CHUNK;
        *dst = realloc(*dst, *dst_len);
        assert(*dst);

        strm.avail_out = UNGZ_CHUNK;
        strm.next_out  = (char *) *dst + *dst_len - UNGZ_CHUNK;

        int zerr = inflate(&strm, Z_NO_FLUSH);
        assert(zerr != Z_STREAM_ERROR);
        if (zerr == Z_NEED_DICT || zerr == Z_DATA_ERROR || zerr == Z_MEM_ERROR)
        {
            log_ef("bad gz: %d", zerr);
            err = 2;
            goto out;
        }
        if (zerr == Z_STREAM_END)
            goto out;
    }

out:
    inflateEnd(&strm);
    if (err)
    {
        free(*dst);
        *dst = NULL;
    }
    return err;
}

// gz ^=========================================================================
