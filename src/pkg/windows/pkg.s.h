#ifndef __PKG_S_H
#define __PKG_S_H

extern const char __pkg_jre;
extern const char __pkg_jre_end;
extern const char __pkg_jrepath;
extern const char __pkg_jrepath_end;
extern const char __pkg_runner;
extern const char __pkg_runner_end;

static const char *pkg_jre         = &__pkg_jre;
static const char *pkg_jre_end     = &__pkg_jre_end;
static const char *pkg_jrepath     = &__pkg_jrepath;
static const char *pkg_jrepath_end = &__pkg_jrepath_end;
static const char *pkg_runner      = &__pkg_runner;
static const char *pkg_runner_end  = &__pkg_runner_end;

#define assert_pkg() \
    do {                                                                                                               \
        assert(pkg_jre     && pkg_jre < pkg_jre_end);                                                                  \
        assert(pkg_jrepath && pkg_jrepath < pkg_jrepath_end);                                                          \
        assert(pkg_runner  && pkg_runner < pkg_runner_end);                                                            \
    } while (0);

#endif
