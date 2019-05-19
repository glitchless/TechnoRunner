#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "pkg_generic.h"
#include "macos/pkg.s.h"

static char *
resolve_minecraft_path()
{
    struct passwd *pw = getpwuid(getuid());

    char *path = NULL;
    asprintf(&path, "%s/%s/%s/%s", pw->pw_dir, "Library", "Application Support", "minecraft");

    return path;
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
