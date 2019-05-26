.global _pkg_jre
.global _pkg_jre_end
.global _pkg_jrepath
.global _pkg_jrepath_end
.global _pkg_runner
.global _pkg_runner_end


.data

_pkg_jre:
.incbin "macos/links/jre.tar.gz"

_pkg_jre_end:
.byte 0

_pkg_jrepath:
.incbin "macos/links/jrepath.txt"

_pkg_jrepath_end:
.byte 0

_pkg_runner:
.incbin "macos/links/runner.jar"

_pkg_runner_end:
.byte 0
