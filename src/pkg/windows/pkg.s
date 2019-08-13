.global __pkg_jre
.global __pkg_jre_end
.global __pkg_jrepath
.global __pkg_jrepath_end
.global __pkg_runner
.global __pkg_runner_end


.data

__pkg_jre:
.incbin "windows\links\jre.tar.gz"

__pkg_jre_end:
.byte 0

__pkg_jrepath:
.incbin "windows\links\jrepath.txt"

__pkg_jrepath_end:
.byte 0

__pkg_runner:
.incbin "windows\links\runner.jar"

__pkg_runner_end:
.byte 0
