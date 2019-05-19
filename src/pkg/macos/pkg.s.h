extern const char _pkg_jre;
extern const char _pkg_jre_end;
extern const char _pkg_jrepath;
extern const char _pkg_jrepath_end;
extern const char _pkg_runner;
extern const char _pkg_runner_end;

static const void *pkg_jre         = &_pkg_jre;
static const void *pkg_jre_end     = &_pkg_jre_end;
static const void *pkg_jrepath     = &_pkg_jrepath;
static const void *pkg_jrepath_end = &_pkg_jrepath_end;
static const void *pkg_runner      = &_pkg_runner;
static const void *pkg_runner_end  = &_pkg_runner_end;

#define assert_pkg() \
    do {                                                                                                               \
        assert(pkg_jre     && pkg_jre < pkg_jre_end);                                                                  \
        assert(pkg_jrepath && pkg_jrepath < pkg_jrepath_end);                                                          \
        assert(pkg_runner  && pkg_runner < pkg_runner_end);                                                            \
    } while (0);
