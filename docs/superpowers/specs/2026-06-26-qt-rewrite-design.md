# TechnoRunner → Qt (C++) Rewrite — Design

**Date:** 2026-06-26
**Status:** Approved (design); spec under review
**Author:** Nikita Kulikov (with Claude Code)

## 1. Goal & Motivation

TechnoRunner is the bootstrapper for the Technopark / glitchless Minecraft launcher.
Today it ships in **two layers**:

1. **Kotlin/Swing runner** (`src/main/`) — shows a splash, downloads an OS/arch-matched
   JRE and the launcher JAR, verifies the JAR's SHA-256, then launches
   `java -jar launcher.jar` detached and exits.
2. **Native C/asm bootstrap** (`src/pkg/`) — a per-platform self-extracting binary that
   embeds a JRE + the runner JAR and unpacks them, *whose only reason to exist is to ship
   the JVM runner as a native executable* so end users don't need Java to start the runner.

**This rewrite reimplements the runner as a native C++/Qt 6 application.** Because a Qt
binary is already a self-contained native executable, the entire native bootstrap layer
(layer 2) becomes unnecessary and is **deleted**. The shipped artifact becomes a single
Qt executable that, on first run, downloads the JRE + launcher JAR at runtime (exactly as
the Kotlin runner does today) and launches the launcher.

### Decisions (locked)

| Decision | Choice |
|---|---|
| Binding / language | **C++ with Qt Widgets** |
| Qt version | **Qt 6** |
| Build system | **CMake** (≥ 3.21) |
| Shipped artifact | **Single self-contained native binary** per platform |
| Native `src/pkg/` bootstrap | **Deleted** |
| Fidelity | **Same external contract** (URLs, dir layout, SHA-256, backoff) **+ flagged improvements** |
| Control-flow model | **Worker-thread imperative port** (Approach A) |
| Archive extraction | **libarchive** (one dependency covering `.tar.gz` + `.zip`) |
| CI | **GitHub Actions** cross-platform, replacing Travis |
| Dependency sourcing | Qt via `aqtinstall` / `install-qt-action`; libarchive via **vcpkg** manifest |
| Release artifacts | Linux **AppImage**, Windows **zip**, macOS **.dmg** (x64 + arm64) |

## 2. External Contract (must remain identical)

The rewrite is a drop-in replacement. The following MUST NOT change:

- **Endpoints:**
  - JRE index: `https://minecraft.glitchless.ru/jres.json`
  - Launcher manifest: `https://minecraft.glitchless.ru/launcher.json`
- **Working directory layout** under the platform app dir: `technomine/`
  - `tmp/` — scratch downloads (`jre.<ext>`, `update_launcher.jar`)
  - `jre/` — extracted JRE
  - `jrepath.txt` — absolute path to the `java` binary
  - `launcher.jar` — the launcher
  - `launcherout.log`, `launchererr.log` — launcher stdout/stderr
- **`jres.json` schema** (array): `type`, `arch`, `downloadUrl`, `javaRelativePath`, `extension`.
  Matching is by `type` ∈ {`Linux`,`Windows`,`macOS`} and `arch` ∈ {`x86_64`,`arm`}.
- **`launcher.json` schema** (object): `version`, `downloadFullPath`, `SHA-256`
  (the hash is **Base64-encoded SHA-256**, e.g. `K6qafxioI7LGPcOeE8ZZrljmGenIfg860yPvjPV6JeM=`).
- **Retry policy:** up to **10 attempts**, exponential backoff `1000 · 2^attempt` ms,
  with a per-second countdown shown in the status line.
- **Launch:** `<java> -jar <launcher.jar>`, working dir = `technomine/`, **detached**,
  stdout→`launcherout.log`, stderr→`launchererr.log`; then the runner exits with code 0.
- **UI strings (Russian) preserved:** "Загрузка...", "Загрузка Java...",
  "Скачивание лаунчера...", "Ошибка при загрузке. Проверьте подключение интернета",
  "Ошибка при загрузке. Попытка N/M (Хс)".

## 3. Architecture

**Approach A — worker-thread imperative port.** A `Bootstrapper` object is moved to a
background `QThread` and runs the same linear sequence as `Main.kt` +
`checkAndDownloadAll.kt`, top to bottom. It emits `status(QString)` and
`progress(int current, int max)` signals to the `SplashScreen` on the UI thread via
`Qt::QueuedConnection`. Downloads use `QNetworkAccessManager` with a small synchronous
helper (a local `QEventLoop`) so the JRE→launcher sequence and the backoff/sleep loop stay
readable, blocking code — never blocking the UI thread.

```
main() [UI thread]
  ├─ QApplication
  ├─ SplashScreen (frameless QWidget), shown + centered
  ├─ QThread + Bootstrapper (moveToThread)
  │     signals: status / progress  ──QueuedConnection──▶ SplashScreen slots
  │     signal:  finished           ──QueuedConnection──▶ onBootstrapFinished()
  └─ onBootstrapFinished(): SplashScreen::stop() → Launcher::run() → qApp->exit(0)

Bootstrapper::run() [worker thread]
  tryExponential(10):
    Platform: detect {type, arch}
    if jrepath.txt missing/empty/dangling:
        JavaDownloader.init()      → GET jres.json, parse, find match
        JavaDownloader.download()  → tmp/jre.<ext>, extract → jre/, return java path
        Paths.writeJREPath(absolute java path)
    LauncherDownloader.init()      → GET launcher.json
    if !LauncherDownloader.checkFile():
        LauncherDownloader.update()→ tmp/update_launcher.jar, verify sha256, replace launcher.jar
  on total failure: status("Ошибка при загрузке. Проверьте подключение интернета")
  emit finished()
```

## 4. Components

Each component maps ~1:1 onto an existing source file.

| New (C++) | Replaces | Responsibility |
|---|---|---|
| `app/ProgressMonitor.h` | `IProgressMonitor` | Abstract Q_OBJECT interface: `setProgress`, `setMax`, `incrementProgress`, `setStatus` (as signals/slots). |
| `app/Bootstrapper.{h,cpp}` | `Main.kt`, `checkAndDownloadAll.kt` | Orchestrates the linear flow + `tryExponential` backoff on a worker thread. |
| `net/Downloader.{h,cpp}` | `HttpUtils`, `FileUtils` | `httpGet(url)->QByteArray` for JSON; `download(url, file, monitor)` streaming `readyRead`→file with `downloadProgress`→monitor; follows redirects; blocks on local `QEventLoop`. |
| `net/JavaDownloader.{h,cpp}` | `JavaDownloader.kt` | Fetch+parse `jres.json`, select OS/arch match, download to `tmp/jre.<ext>`, extract to `jre/`, return `jre/<javaRelativePath>`. |
| `net/LauncherDownloader.{h,cpp}` | `LauncherDownloader.kt` | Fetch+parse `launcher.json`; `checkFile()` SHA-256 compare; atomic download→verify→replace. |
| `models/JavaBinaryModel.h` | gson data class | Struct + `QJson` parse. |
| `models/LauncherModel.h` | gson data class | Struct + `QJson` parse (`"SHA-256"` key → `sha256`). |
| `util/Paths.{h,cpp}` | `DirectoryHelper.kt` | Resolve `technomine/` base + all sub-paths; `writeJREPath`/`getJREPath` with the same fallback to `"java"`. |
| `util/Hash.{h,cpp}` | `HashUtils.java` | `QCryptographicHash::Sha256` → `toBase64()` (string-equal to today's output). |
| `util/Platform.{h,cpp}` | `oslib` usage | Normalize current OS/arch to the `jres.json` `type`/`arch` vocabulary (`arm64`→`arm`). |
| `util/Archive.{h,cpp}` | `jarchivelib` | libarchive wrapper: `extractZip` and `extractTarGz` into a destination dir. |
| `run/Launcher.{h,cpp}` | `runLauncher.kt` | Build command, set working dir, **detached** launch with log redirection. |
| `ui/SplashScreen.{h,cpp}` | `SplashScreen.kt` | Frameless splash; implements the monitor slots. |
| `ui/GProgressBar.{h,cpp}` | `GProgressBar.kt` | Rounded green progress bar. |
| `main.cpp` | (top of `Main.kt`) | Wire everything, start the worker, run the event loop. |
| `technorunner-hasher` (optional target) | `HasherMain.kt` | Dev utility: print Base64 SHA-256 of a file. Behind a CMake option. |

### 4.1 Notable component details

- **Paths base dir — RESOLVED** (from `mclauncher-api` `impl/common/*OS.java` at ref
  `cf44fb59f4`). The base is `<workingDirectory>/technomine`, where `workingDirectory` is,
  **per OS, replicated literally** (do NOT use `QStandardPaths`, which appends app names —
  use `QDir::homePath()` + the literal env var):

  | OS | `workingDirectory` | → `technomine` base |
  |---|---|---|
  | Linux/Unix | `$HOME/.minecraft` | `$HOME/.minecraft/technomine` |
  | Windows | `%APPDATA%\.minecraft` (fallback `$HOME\.minecraft` if `APPDATA` unset) | `%APPDATA%\.minecraft\technomine` |
  | macOS | `$HOME/Library/Application Support/minecraft` (**no leading dot**) | `$HOME/Library/Application Support/minecraft/technomine` |

  OS is selected first-match in order Windows→Mac→Linux by `os.name` substring
  (`win`/`mac`/`linux|unix`). Existing installs therefore keep their current directory.
- **OS/arch matching — RESOLVED** (from `oslib` at ref `d5ba9facde`). For each `jres.json`
  entry, the original computes `OperatingSystem.getOperatingSystem(entry.type) ==
  currentOS.type && Arch.getArch(entry.arch) == currentOS.arch`. The Qt `Platform` util
  replicates:
  - **String → enum** (case-insensitive): `type` `Linux/Windows/macOS` → `LINUX/WINDOWS/MACOS`;
    `arch` matched by exact equals against search lists — `x86_64`={`x86_64`,`amd64`,`k8`},
    `ARM`={`ARM`,`arm64`}, `x86`={`x86`,`i386`,`i486`,`i586`,`i686`}, else `UNKNOWN`. (Note
    `jres.json`'s `"arm"` matches `ARM` via `"ARM"` case-insensitively; `"aarch64"` would
    NOT match anything — irrelevant since only macOS has an arm entry.)
  - **Current-machine arch** (must match the original's detection, not just the binary's
    build arch):
    - **macOS:** if `sysctlbyname("sysctl.proc_translated") == 1` (running under Rosetta) →
      `ARM`; else `uname -m` (`arm64`→`ARM`, `x86_64`→`x86_64`). This makes an
      Apple-Silicon machine select the macOS **arm** JRE whether the runner is native or
      x86-emulated.
    - **Windows:** if env `ProgramFiles(x86)` is set → `x86_64`; else parse `os.arch`
      equivalent. (Only an x86_64 entry exists.)
    - **Linux:** `uname -m` equivalent via `QSysInfo::currentCpuArchitecture()`. (Only an
      x86_64 entry exists.)
- **Launcher detached + log redirection — design note.** `QProcess::startDetached` does not
  carry stdout/stderr file redirection. To preserve `launcherout.log`/`launchererr.log`,
  launch through a per-OS shim:
  - Unix: `QProcess::startDetached("/bin/sh", {"-c", "exec \"$JAVA\" -jar \"$JAR\" > \"$OUT\" 2> \"$ERR\""}, workingDir)`
  - Windows: `QProcess::startDetached("cmd.exe", {"/C", "\"<java>\" -jar \"<jar>\" > <out> 2> <err>"}, workingDir)`
  Careful quoting of paths with spaces is required and covered by tests where feasible.
- **Downloader streaming.** Write the JRE archive to disk as it arrives (`readyRead`),
  never buffering the whole file in memory — matches `FileUtils.downloadFileWithProgress`.
- **TLS on Windows — VERIFY.** Qt 6 ships a Schannel TLS backend, so HTTPS should work
  without bundling OpenSSL. Confirm during implementation; fall back to bundling OpenSSL
  if needed.

## 5. UI Fidelity

Frameless top-level `QWidget` (`Qt::FramelessWindowHint`) sized to `background.jpeg` and
centered on the primary screen.

- **Background:** `QLabel` with `:/background.jpeg` (from Qt resources).
- **Close button:** `QLabel`/`QPushButton` with `:/close-btn.png`, positioned at
  `(bgWidth − btnWidth − 13, 13)`, hand cursor, click → `exit(0)`.
- **Status bar:** background `#303135`; centered `QLabel`, color `#ddddde`,
  font Helvetica/Arial 14, padding (15, 20, 8, 20), default text "Загрузка...".
- **Progress bar (GProgressBar):** track `#ddddde`, fill `#00db9d`, corner radius 4,
  height 20, padding (0, 20, 15, 20). Implemented via QSS
  (`QProgressBar::chunk { background:#00db9d; border-radius:4px }`); a `paintEvent`
  override is the fallback if pixel-exact rounding on both ends is required.

## 6. Error Handling

Mirrors the original exactly:

- Each `tryExponential` attempt runs the block in a try/catch; on exception it logs (to the
  runner log — see §8), waits `1000 · 2^attempt` ms while updating the status line with a
  live per-second countdown ("Ошибка при загрузке. Попытка N/M (Хс)"), then retries.
- After 10 failed attempts: status "Ошибка при загрузке. Проверьте подключение интернета",
  then the runner still proceeds to the launch+exit path (matching current behavior).
- SHA-256 mismatch on the downloaded launcher aborts the replace, keeping the existing
  `launcher.jar`.
- A `null` JRE match (no entry for this OS/arch) returns without downloading.

## 7. Build System & Dependencies

- **CMake** (≥ 3.21, C++17):
  - `find_package(Qt6 REQUIRED COMPONENTS Widgets Network)`; `Qt6::Test` for tests.
  - `find_package(LibArchive REQUIRED)`.
  - `qt_standard_project_setup()`, `qt_add_executable(TechnoRunner ...)`,
    `qt_add_resources(... resources/resources.qrc)`.
  - `set_target_properties(TechnoRunner PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)`
    (no console window on Windows; `.app` on macOS, with `icon.icns`).
  - Optional `technorunner-hasher` target behind a CMake option (default OFF).
- **Dependencies:**
  - **Qt 6** installed in CI via `aqtinstall` / `jurplel/install-qt-action`.
  - **libarchive** via a `vcpkg.json` manifest (covers `.tar.gz` + `.zip`), statically
    linked where practical for a self-contained binary.

## 8. Flagged Improvements (kept minimal; "port + improvements")

1. **Single self-contained native binary** — removes the JRE-bootstrap layer entirely (the headline change).
2. **Robust detached launch** via `QProcess` instead of brittle `nohup`/`cmd` string-building, with log redirection preserved (§4.1).
3. **Runner-side log file** (`technomine/runner.log`): a windowed app has no console, so the
   original `println`/`printStackTrace` output would vanish. Route runner diagnostics
   (and caught exceptions) to this file.
4. **HiDPI:** Qt 6 high-DPI scaling is on by default. The raster assets may look soft on
   HiDPI displays; optional `@2x` assets are noted as future work and do **not** block this work.
5. **Streaming download to disk + atomic JAR replacement** — preserves existing semantics,
   made explicit.

Explicitly **out of scope** (parity-neutral extras): draggable splash, Esc-to-close,
macOS code signing / notarization (unsigned `.app`/`.dmg` will trigger a Gatekeeper warning).

## 9. CI / Packaging

`.github/workflows/build.yml` (triggers: push, PR, and tags), replacing `.travis.yml`.
Matrix:

| Runner | Arch | Deploy step | Artifact |
|---|---|---|---|
| `ubuntu-latest` | x86_64 | `linuxdeploy` + qt plugin | `TechnoRunner-linux-x86_64.AppImage` |
| `windows-latest` | x86_64 | `windeployqt` | `TechnoRunner-windows-x86_64.zip` |
| `macos-13` | x86_64 | `macdeployqt` → `.dmg` (reuse `scripts/Background.png`) | `TechnoRunner-macos-x86_64.dmg` |
| `macos-14` | arm64 | `macdeployqt` → `.dmg` | `TechnoRunner-macos-arm64.dmg` |

Each job: checkout → install Qt (aqtinstall) → install libarchive (vcpkg) → CMake
configure+build → run `ctest` → deploy/bundle → upload artifact. On tags, attach artifacts
to a GitHub Release. macOS artifacts are unsigned (notarization out of scope).

**macOS bundle identity — reuse the old `Info.plist`** (`src/pkg/macos/app/Info.plist`):

- `CFBundleIdentifier` = `ru.glitchless.games`
- `CFBundleExecutable` = `glitchless-minecraft`
- `CFBundleIconFile` = `icon.icns` (reuse `src/pkg/macos/app/Resources/icon.icns`)
- `NSHighResolutionCapable` = `true`
- `LSUIElement` = `true` (agent app — no Dock icon / menu bar; correct for a splash launcher)
- `.app` bundle name = `Minecraft.app` (as today)

These are applied via CMake `MACOSX_BUNDLE_*` properties / a `MACOSX_BUNDLE_INFO_PLIST`
template so the bundle matches the current release.

## 10. Testing Strategy

Qt Test targets, run via `ctest` in CI:

- **Hash:** known SHA-256→Base64 vectors.
- **Platform:** table-driven OS/arch normalization → `jres.json` vocabulary.
- **Models:** parse the repo's own `jres.json` / `launcher.json` as fixtures.
- **JavaDownloader match:** given a parsed list + faked platform, picks the correct entry
  (incl. macOS arm vs x64).
- **LauncherDownloader.checkFile:** temp files with known hashes (match / mismatch / missing / no-model).
- **Archive:** extract a tiny known `.zip` and `.tar.gz` fixture; assert files land correctly.
- **Paths:** base-dir resolution with mocked `HOME`/`APPDATA`.

Network, process launch, and GUI are kept thin: a `QTcpServer`-backed fake may cover the
`Downloader`; `SplashScreen` gets an instantiate/show-offscreen smoke test; the rest is
manual verification.

## 11. Migration Plan

Work on branch `qt-rewrite` (history preserved).

**Delete:** `src/main/` (Kotlin/Java), `src/pkg/` (all C/asm), `build.gradle`,
`settings.gradle`, `gradle/`, `gradlew`, `gradlew.bat`, `linux.Makefile`, `macos.Makefile`,
`windows.Makefile`, `.travis.yml`, `scripts/macos_pkg_dmg.sh`.

**Keep:** `scripts/Background.png` (DMG background), `src/pkg/macos/app/Info.plist` +
`Resources/icon.icns` (adapted for the macOS bundle — copy out before deleting `src/pkg`),
`jres.json` / `launcher.json` (server-side reference copies; not part of the build),
`README.md` (updated).

**Add:** `CMakeLists.txt`, `vcpkg.json`, `src/` (C++ tree), `resources/` (qrc + the two
images moved from `src/main/resources/`), `tests/`, `.github/workflows/build.yml`.

## 12. Open Items to Verify During Implementation

Resolved during design (see §4.1, §9): exact `technomine` base dir, OS/arch detection &
matching, and macOS bundle identity — all read from the pinned dependency sources.

Remaining:

1. **Windows TLS** — confirm Qt 6 Schannel backend handles HTTPS without bundled OpenSSL.
2. **libarchive packaging** per platform in CI (vcpkg triplets; static vs dynamic).
3. **QProcess detached + redirection quoting** per OS (paths with spaces).
4. **Qt deployment size** (~20–40 MB with Widgets+Network) — acceptable, still far smaller
   than embedding a JRE.

## 13. Addendum (2026-06-26): JRE moved into `launcher.json`

The server consolidated the JRE descriptor into the launcher manifest, superseding the
separate `jres.json` endpoint:

```jsonc
{
  "version": "...", "downloadFullPath": "...", "SHA-256": "...",
  "jre": {
    "code": "jre8_202",                 // names the install subfolder
    "files": [ { "type","arch","extension","downloadUrl","javaRelativePath" }, ... ]
  }
}
```

Contract changes:
- The runner fetches **only** `launcher.json` (one request); the `jres.json` endpoint and
  its committed reference copy are removed.
- The JRE is extracted into a **code-named subfolder**: `<base>/technomine/jre/<code>`
  (e.g. `.../jre/jre8_202`), instead of the flat `jre/`. `jrepath.txt` stores the absolute
  path to the `java` binary inside it.
- `LauncherModel` now also carries `jreCode` + `jreFiles`; `JavaDownloader::setCandidates(code, files)`
  replaces the old `init()` that fetched `jres.json`. Flow order is now: fetch manifest →
  (if needed) download JRE → check/replace launcher jar.
- `needJre` is still decided by `jrepath.txt` (missing/empty/dangling), unchanged.
