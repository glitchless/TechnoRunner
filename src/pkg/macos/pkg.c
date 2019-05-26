#include <stdlib.h>
#include <assert.h>

#include "pkg_generic.h"
#include "util.h"

#include "macos/pkg.s.h"

int
main()
{
    assert_pkg();

    char *minecraft_path = join_path(get_home_path(), "Library", "Application Support", "minecraft", "technomine", NULL);

    int err = pkg_main(
            pkg_jre,     pkg_jre_end - pkg_jre,
            pkg_jrepath, pkg_jrepath_end - pkg_jrepath,
            pkg_runner,  pkg_runner_end - pkg_runner,
            minecraft_path);

    free(minecraft_path);
    return err;
}
