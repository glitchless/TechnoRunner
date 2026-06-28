# Overlay triplet for the HOST tools vcpkg builds while compiling Qt (build-time
# helpers such as openssl, zlib, dbus and the vcpkg-* tools). Identical to the
# builtin x64-windows (dynamic) triplet, but release-only: by default the host
# graph builds BOTH debug and release of every package -- the host openssl debug
# build alone took ~11 min on CI. Dropping the debug configuration roughly halves
# the host build time and disk. vcpkg resolves the host triplet "x64-windows"
# from VCPKG_OVERLAY_TRIPLETS, so this file shadows the builtin automatically.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_BUILD_TYPE release)
