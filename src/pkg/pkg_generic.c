#include "pkg_generic.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "util.h"
#include "paths.h"

static int
handle_jre(const char *buf, size_t buf_len, const char *dest_path)
{
    assert(buf && dest_path);

    int err = 0;
    int ierr = 0;
    char *jre_path = NULL;
    char *jrepath_path = NULL;
    char *ungz_buf = NULL;
    size_t ungz_buf_len = 0;

    jrepath_path = join_path(dest_path, JREPATH_FILENAME, NULL);
    if (check_file_exist(jrepath_path))
        goto out;

    jre_path = join_path(dest_path, JRE_DIRNAME, NULL);

    log_if("start unpack to '%s'", jre_path);

    if (ierr = ungz(buf, buf_len, &ungz_buf, &ungz_buf_len))
    {
        log_ef("can't ungz jre.tar.gz: %d", ierr);
        err = 1;
        goto out;
    }

    if (ierr = untar(ungz_buf, ungz_buf_len, jre_path))
    {
        log_ef("can't untar jre.tar.gz to '%s': %d", jre_path, ierr);
        err = 2;
        goto out;
    }

out:
    if (err)
        remove(jrepath_path);

    free(jre_path);
    free(jrepath_path);
    free(ungz_buf);
    return err;
}

static int
handle_jrepath(const char *buf, size_t buf_len, const char *dest_path)
{
    assert(buf && dest_path);

    int err = 0;
    char *jrepath_path = NULL;
    FILE *f = NULL;

    jrepath_path = join_path(dest_path, JREPATH_FILENAME, NULL);
    if (check_file_exist(jrepath_path))
        goto out;

    log_if("start unpack to '%s'", jrepath_path);

    if (mkdirp_for_file(jrepath_path))
    {
        log_ef("can't create parent dir for '%s'", jrepath_path);
        err = 1;
        goto out;
    }

    f = fopen(jrepath_path, "wb");
    if (!f)
    {
        log_ef("can't open file '%s'", jrepath_path);
        err = 2;
        goto out;
    }

    fwrite(buf, 1, buf_len, f);
    if (ferror(f))
    {
        log_ef("can't write file '%s'", jrepath_path);
        err = 3;
        goto out;
    }

out:
    if (err)
        remove(jrepath_path);

    free(jrepath_path);
    if (f)
        fclose(f);
    return err;
}

static int
handle_runner(const char *buf, size_t buf_len, const char *dest_path)
{
    assert(buf && dest_path);

    int err = 0;
    char *runner_path = NULL;
    FILE *f = NULL;

    runner_path = join_path(dest_path, RUNNER_JAR_FILENAME, NULL);
    if (check_file_exist(runner_path))
        goto out;

    log_if("start unpack to '%s'", runner_path);

    if (mkdirp_for_file(runner_path))
    {
        log_ef("can't create parent dir for '%s'", runner_path);
        err = 1;
        goto out;
    }

    f = fopen(runner_path, "wb");
    if (!f)
    {
        log_ef("can't open file '%s'", runner_path);
        err = 2;
        goto out;
    }

    fwrite(buf, 1, buf_len, f);
    if (ferror(f))
    {
        log_ef("can't write file '%s'", runner_path);
        err = 3;
        goto out;
    }

out:
    if (err)
        remove(runner_path);

    free(runner_path);
    if (f)
        fclose(f);
    return err;
}

static int
run_runner(const char *root_path)
{
    assert(root_path);

    int err = 0;
    char *java_path = NULL;
    char *runner_path = NULL;

    java_path = join_path(root_path, JRE_DIRNAME, "jre1.8.0_202", "bin", "java", NULL);
    if (!check_file_exist(java_path))
    {
        log_ef("no java at '%s'", java_path);
        err = 1;
        goto out;
    }

    if (chmod(java_path, 0755))
    {
        log_ef("can't chmod 755 java '%s'", java_path);
        err = 2;
        goto out;
    }

    runner_path = join_path(root_path, RUNNER_JAR_FILENAME, NULL);
    if (!check_file_exist(runner_path))
    {
        log_ef("no runner jar at '%s'", runner_path);
        err = 3;
        goto out;
    }

    signal(SIGHUP, SIG_IGN);

    log_if("forking to run '%s -jar %s'", java_path, runner_path);
    pid_t p = fork();
    if (p < 0)
    {
        log_ef("can't fork: %d ('%s')", errno, strerror(errno));
        err = 4;
        goto out;
    }

    if (p == 0)
    {
        execl(java_path, "-jar", runner_path, NULL);
    }

out:
    free(java_path);
    free(runner_path);
    return err;
}

int
pkg_main(
        const char *jre_buf, size_t jre_buf_len,
        const char *jrepath_buf, size_t jrepath_buf_len,
        const char *runner_buf, size_t runner_buf_len,
        const char *dest_path)
{
    assert(jre_buf && jrepath_buf && runner_buf && dest_path);

    int ierr = 0;

    if (ierr = handle_jre(jre_buf, jre_buf_len, dest_path))
    {
        log_ef("jre failed: %d", ierr);
        return 1;
    }

    if (ierr = handle_jrepath(jrepath_buf, jrepath_buf_len, dest_path))
    {
        log_ef("jrepath failed: %d", ierr);
        return 2;
    }

    if (ierr = handle_runner(runner_buf, runner_buf_len, dest_path))
    {
        log_ef("runner failed: %d", ierr);
        return 3;
    }

    if (ierr = run_runner(dest_path))
    {
        log_ef("can't run runner: %d", ierr);
        return 4;
    }

    return 0;
}

