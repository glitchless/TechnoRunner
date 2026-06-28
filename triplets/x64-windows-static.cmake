# Overlay triplet: identical to vcpkg's builtin x64-windows-static, but
# release-only (VCPKG_BUILD_TYPE release). The default static triplet builds
# BOTH debug and release of every port; CI only ships Release, so skipping the
# debug configuration roughly halves both the from-source Qt build time and its
# peak disk usage -- the latter is what overran the windows-2025 runner's ~33 GB
# C: drive (no D: drive since 2025-07). Same filename as the builtin triplet so
# it shadows it when this directory is passed via VCPKG_OVERLAY_TRIPLETS.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)
