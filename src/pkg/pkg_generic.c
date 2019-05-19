#include "pkg_generic.h"

#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "ungz.h"
#include "untar.h"
#include "paths.h"

int
handle_jre(const void *buf, size_t buf_len, const char *dest_path)
{
    assert(buf && dest_path);

    int err = 0;
    int ierr = 0;
    char *jre_path = NULL;
    void *ungz_buf = NULL;
    size_t ungz_buf_len = 0;

    jre_path = join_path(dest_path, JRE_DIRNAME, NULL);
    if (check_file_exist(jre_path))
        goto out;

    if (!(ierr = ungz(buf, buf_len, &ungz_buf, &ungz_buf_len)))
    {
        printf("error ungz jre.tar.gz: %d\n", ierr);
        err = 1;
        goto out;
    }

    if (!(ierr = untar(ungz_buf, ungz_buf_len, jre_path)))
    {
        printf("error untar jre.tar.gz to %s: %d\n", jre_path, ierr);
        err = 2;
        goto out;
    }

out:
    free(jre_path);
    if (f)
        fclose(f);
    free(ungz_buf);
    return err;
}

int
handle_jrepath(const void *buf, size_t buf_len, const char *dest_path)
{
    assert(buf && dest_path);

    int err = 0;
    char *jrepath_path = NULL;
    FILE *f = NULL;

    jrepath_path = join_path(dest_path, JREPATH_FILENAME, NULL);
    if (check_file_exist(jrepath_path))
        goto out;

    f = fopen(jrepath_path, "wb");
    if (!f)
    {
        printf("error opening %s\n", jrepath_path);
        err = 1;
        goto out;
    }

    fwrite(buf, 1, buf_len, f);
    if (ferror(f))
    {
        printf("error writing %s\n", jrepath_path);
        err = 2;
        goto out;
    }

out:
    free(jrepath_path);
    if (f)
        fclose(f);
    return err;
}

int
handle_runner(const void *buf, size_t buf_len, const char *dest_path)
{
    assert(buf && dest_path);

    int err = 0;
    char *runner_path = NULL;
    FILE *f = NULL;

    runner_path = join_path(dest_path, RUNNER_FILENAME, NULL);
    if (check_file_exist(runner_path))
        goto out;

    f = fopen(runner_path, "wb");
    if (!f)
    {
        printf("error opening %s\n", runner_path);
        err = 1;
        goto out;
    }

    fwrite(buf, 1, buf_len, f);
    if (ferror(f))
    {
        printf("error writing %s\n", runner_path);
        err = 2;
        goto out;
    }

out:
    free(runner_path);
    if (f)
        fclose(f);
    return err;
}

int
handle_all(
        const void *jre_buf, size_t jre_buf_len,
        const void *jrepath_buf, size_t jrepath_buf_len,
        const void *runner_buf, size_t runner_buf_len,
        const char *dest_path)
{
    assert(jre_buf && jrepath_buf && runner_buf && dest_path);

    int ierr = 0;

    if (ierr = handle_jre(jre_buf, jre_buf_len, dest_path))
    {
        printf("jre failed: %d\n", ierr);
        return 1;
    }

    if (ierr = handle_jrepath(jrepath_buf, jrepath_buf_len, dest_path))
    {
        printf("jrepath failed: %d\n", ierr);
        return 2;
    }

    if (ierr = handle_runner(runner_buf, runner_buf_len, dest_path))
    {
        printf("runner failed: %d\n", ier);
        return 3;
    }

    return 0;
}

