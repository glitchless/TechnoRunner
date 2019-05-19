#include "untar.h"

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

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
untar(const void *buf, size_t buf_len, const char *dest_path)
{
    if (!buf || !dest_path)
        return 0;

    for (void *buff = buf; buff < buf + buf_len; buff += 512)
    {
        if ((buff - buf) < 512)
            return 1;
        if (is_end_of_archive(buff))
            return 0;
        if (!verify_checksum(buff))
            return 2;

        FILE *f = NULL;
        int filesize = parseoct(buff + 124, 12);
        switch (buff[156])
        {
            case '1':
            case '2':
            case '3':
            case '4':
            case '6':
                break;
            case '5':
                if (mkdirp(buff))
                    return 3;
                filesize = 0;
                break;
            default:
                if (mkdirp_for_file(buff))
                    return 4;
                f = fopen(buff, "wb");
        }

        if (f && filesize)
        {
            int r = fwrite(buff, 1, filesize, f);
            fclose(f);
            f = NULL;

            if (r != filesize)
                return 5;
        }
    }

    return 0;
}
