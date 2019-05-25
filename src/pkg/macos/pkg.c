#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <assert.h>

#include "macos/pkg.s.h"

#include "pkg_generic.h"
#include "util.h"

static char *
resolve_minecraft_path()
{
    struct passwd *pw = getpwuid(getuid());
    char *home_dir = pw->pw_dir;

    return join_path(
            home_dir, "Library", "Application Support", "minecraft", "technomine",
            NULL);
}

int
main()
{
    assert_pkg();

    char *minecraft_path = resolve_minecraft_path();

    int err = handle_all(
            pkg_jre,     pkg_jre_end - pkg_jre,
            pkg_jrepath, pkg_jrepath_end - pkg_jrepath,
            pkg_runner,  pkg_runner_end - pkg_runner,
            minecraft_path);

    free(minecraft_path);
    return err;
}
