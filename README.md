# TechnoRunner

Native (C++/Qt 6) bootstrapper for the Technopark / glitchless Minecraft launcher.
On first run it downloads an OS/arch-matched JRE and the launcher JAR into
`<app-data>/.minecraft/technomine`, verifies the JAR's SHA-256, then launches it.

## Build

Requires Qt 6 (Widgets, Network, Test), CMake ≥ 3.21, and libarchive.

```bash
# Linux: deps via apt — sudo apt install cmake qt6-base-dev qt6-base-dev-tools libarchive-dev
# Windows: libarchive via vcpkg (see vcpkg.json); pass the vcpkg toolchain file to cmake.
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure   # set QT_QPA_PLATFORM=offscreen when headless
```

Run: `./build/TechnoRunner`

## Running a release build on macOS

The `.dmg` ships an ad-hoc signed (but **not notarized**) `Minecraft.app`. The bundle's
dylibs/frameworks are validly signed, so once the download quarantine flag is cleared the
app launches on any Apple Silicon Mac. After dragging it to **Applications**:

```bash
xattr -cr /Applications/Minecraft.app   # clear the download quarantine flag, then open it
```

Right-clicking the app → **Open** also works on most macOS versions. (Full Gatekeeper
trust without this step would require a Developer ID certificate + notarization.)

## Layout

- `src/` — application sources (`util/`, `models/`, `net/`, `run/`, `app/`, `ui/`, `main.cpp`)
- `tests/` — Qt Test unit tests (one executable per component)
- `resources/` — embedded images (`background.jpeg`, `close-btn.png`) + macOS bundle assets
- `.github/workflows/build.yml` — cross-platform build/test/release (Linux AppImage, Windows zip, macOS dmg)

History (Kotlin/Swing runner + native C bootstrap) lives in git before the `qt-rewrite` branch.
