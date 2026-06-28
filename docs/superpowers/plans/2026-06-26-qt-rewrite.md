# TechnoRunner Qt Rewrite — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reimplement the TechnoRunner Minecraft-launcher bootstrapper as a single self-contained native C++/Qt 6 application, deleting the old Kotlin/Swing runner and the native C/asm packaging layer.

**Architecture:** A frameless `QWidget` splash on the UI thread; a `Bootstrapper` (QObject implementing a `ProgressMonitor` interface) runs the linear download→launch flow on a worker `QThread` and emits `status`/`progress` signals to the splash via queued connections. Downloads use `QNetworkAccessManager` with a synchronous (local-`QEventLoop`) helper; archives are unpacked with libarchive; the launcher is started detached via `QProcess`. All logic lives in a static lib `tprunner_lib` so unit tests can link against it.

**Tech Stack:** C++17, Qt 6 (Widgets, Network, Test), CMake ≥ 3.21, libarchive (via vcpkg), GitHub Actions CI.

**Design spec:** `docs/superpowers/specs/2026-06-26-qt-rewrite-design.md` (read it before starting).

## Global Constraints

These apply to **every** task (copied verbatim from the spec):

- Language/toolchain: **C++17**, **Qt 6** (`Widgets`, `Network`, `Test`), **CMake ≥ 3.21**.
- All code in namespace **`tprunner`**.
- Endpoints (do not change): JRE index `https://minecraft.glitchless.ru/jres.json`; launcher manifest `https://minecraft.glitchless.ru/launcher.json`.
- `technomine` base dir = `<workingDirectory>/technomine`, with `workingDirectory` replicated **literally** (use `QDir::homePath()` + literal env var, NOT `QStandardPaths`):
  - Linux/Unix: `$HOME/.minecraft`
  - Windows: `%APPDATA%\.minecraft` (fallback `$HOME\.minecraft` if `APPDATA` unset)
  - macOS: `$HOME/Library/Application Support/minecraft` (**no leading dot**)
- Sub-paths under base: `tmp/`, `jre/`, `jrepath.txt`, `launcher.jar`, `launcherout.log`, `launchererr.log`, plus new `runner.log`.
- SHA-256 is **Base64-encoded** (standard alphabet, with `=` padding) — must string-equal Java's `Base64.getEncoder().encode(sha256)`.
- `launcher.json` keys: `version`, `downloadFullPath`, `SHA-256`. `jres.json` is an array of `{type, arch, downloadUrl, javaRelativePath, extension}`.
- OS/arch matching vocabulary: `type` ∈ {`Linux`,`Windows`,`macOS`}, `arch` ∈ {`x86_64`,`arm`}. Normalize per oslib rules (see Task 3).
- Retry: up to **10** attempts, backoff `2^attempt` seconds, with a per-second countdown status.
- Russian UI strings preserved exactly (see Task 13 / Bootstrapper).
- Launch: `<java> -jar <launcher.jar>`, working dir = base, **detached**, stdout→`launcherout.log`, stderr→`launchererr.log`, then exit 0.
- TDD: every task is failing-test → run(fail) → implement → run(pass) → commit. Frequent commits.

---

## File Structure & Interface Contracts

Build artifacts:
- `tprunner_lib` (STATIC) — every `src/**.cpp` **except** `src/main.cpp`.
- `TechnoRunner` (executable) — `src/main.cpp` + `resources/resources.qrc`, links `tprunner_lib`.
- One test executable per `tests/*Test.cpp`, each linking `tprunner_lib`, registered with CTest.

Files & responsibilities:

| File | Responsibility |
|---|---|
| `CMakeLists.txt` | Project, `tprunner_lib`, exe, tests, deploy props |
| `vcpkg.json` | Pin `libarchive` |
| `src/util/Hash.{h,cpp}` | `QString sha256Base64(const QString& path)` |
| `src/util/Platform.{h,cpp}` | `Os`/`CpuArch` enums; `osFromString`, `archFromString`, `currentOs`, `currentArch`, `matchesCurrent` |
| `src/util/Paths.{h,cpp}` | `Paths` static accessors + pure `workingDirectoryFor` + test override |
| `src/models/Models.{h,cpp}` | `JavaBinaryModel`, `LauncherModel` + JSON parsers |
| `src/util/Archive.{h,cpp}` | `Archive::extractZip`, `Archive::extractTarGz` (libarchive) |
| `src/app/ProgressMonitor.h` | Pure abstract `ProgressMonitor` interface |
| `src/net/Downloader.{h,cpp}` | `httpGet`, `downloadToFile` (sync, progress) |
| `src/net/JavaDownloader.{h,cpp}` | `init`, `hasMatch`, `download`, static `findMatch` |
| `src/net/LauncherDownloader.{h,cpp}` | `init`, `checkFile`, `update` |
| `src/run/Launcher.{h,cpp}` | static `buildCommand`, static `run` |
| `src/app/Bootstrapper.{h,cpp}` | `Bootstrapper` QObject + free `tryExponential` |
| `src/ui/GProgressBar.{h,cpp}` | Styled rounded `QProgressBar` |
| `src/ui/SplashScreen.{h,cpp}` | Frameless splash widget + slots/accessors |
| `src/main.cpp` | Wire everything; run event loop |
| `resources/resources.qrc` | Embeds `background.jpeg`, `close-btn.png` |

**Locked interface signatures** (later tasks rely on these exact names/types):

```cpp
namespace tprunner {

// Hash.h
QString sha256Base64(const QString& filePath);                 // throws std::runtime_error on read failure

// Platform.h
enum class Os { Windows, MacOs, Linux, Solaris, Bsd, Unknown };
enum class CpuArch { X86, X86_64, Arm, Unknown };
Os osFromString(const QString& s);
CpuArch archFromString(const QString& s);
Os currentOs();
CpuArch currentArch();
bool matchesCurrent(const QString& entryType, const QString& entryArch);
QString workingDirectoryForOs(Os os, const QString& home, const QString& appData);

// Paths.h
class Paths {
public:
    static QString workingDirectory();
    static QString baseDirectory();      // ensures dir exists
    static QString temporaryDirectory(); // ensures dir exists
    static QString javaDirectory();      // ensures dir exists
    static QString jrePathFile();
    static void    writeJrePath(const QString& absoluteJavaPath);
    static QString jrePath();            // file contents or "java" fallback
    static QString launcherFile();
    static QString launcherOutLog();
    static QString launcherErrLog();
    static QString runnerLog();
    static void    setBaseOverride(const QString& dir); // tests only ("" clears)
};

// Models.h
struct JavaBinaryModel {
    QString type, arch, downloadUrl, javaRelativePath, extension;
    static QList<JavaBinaryModel> listFromJson(const QByteArray& json);
};
struct LauncherModel {
    QString version, downloadUrl, sha256;
    static std::optional<LauncherModel> fromJson(const QByteArray& json);
};

// Archive.h
class Archive {
public:
    static void extractZip(const QString& archivePath, const QString& destDir);     // throws std::runtime_error
    static void extractTarGz(const QString& archivePath, const QString& destDir);   // throws std::runtime_error
};

// ProgressMonitor.h
class ProgressMonitor {
public:
    virtual ~ProgressMonitor() = default;
    virtual void setProgress(int progress) = 0;
    virtual void setMax(int max) = 0;
    virtual void incrementProgress(int amount) = 0;
    virtual void setStatus(const QString& status) = 0;
};

// Downloader.h
class Downloader {
public:
    explicit Downloader(QNetworkAccessManager* nam);
    QByteArray httpGet(const QString& url);                                          // throws std::runtime_error
    void downloadToFile(const QString& url, const QString& destPath, ProgressMonitor* monitor); // throws
private:
    QNetworkAccessManager* nam_;
};

// JavaDownloader.h
class JavaDownloader {
public:
    explicit JavaDownloader(Downloader* dl);
    void init();                              // fetch+parse jres.json, select match
    bool hasMatch() const;
    QString download(ProgressMonitor* monitor); // returns absolute java path, or "" if no match
    static std::optional<JavaBinaryModel> findMatch(const QList<JavaBinaryModel>& list, Os os, CpuArch arch);
};

// LauncherDownloader.h
class LauncherDownloader {
public:
    explicit LauncherDownloader(Downloader* dl);
    void init();
    bool checkFile();
    void update(ProgressMonitor* monitor);
};

// Launcher.h
struct LaunchCommand { QString program; QStringList arguments; };
class Launcher {
public:
    static LaunchCommand buildCommand(const QString& javaPath, const QString& jarPath,
                                      const QString& outLog, const QString& errLog, Os os);
    static bool run();
};

// Bootstrapper.h
bool tryExponential(int attemptsNumber, ProgressMonitor& monitor,
                    const std::function<void()>& block,
                    const std::function<void(int seconds)>& sleepSeconds);
class Bootstrapper : public QObject, public ProgressMonitor {
    Q_OBJECT
public:
    explicit Bootstrapper(QObject* parent = nullptr);
    void setProgress(int) override;
    void setMax(int) override;
    void incrementProgress(int) override;
    void setStatus(const QString&) override;
public slots:
    void run();
signals:
    void progressChanged(int);
    void maxChanged(int);
    void statusChanged(const QString&);
    void finished();
};

} // namespace tprunner
```

---

## Task 1: Build skeleton (CMake + Qt app + Qt Test harness)

**Files:**
- Create branch `qt-rewrite`
- Create: `CMakeLists.txt`, `vcpkg.json`, `src/main.cpp`, `tests/SmokeTest.cpp`, `cmake/` (none yet), update `.gitignore`

**Interfaces:**
- Produces: a buildable `TechnoRunner` exe and a working CTest harness for all later tasks.

- [ ] **Step 1: Create the branch**

```bash
cd /home/lionzxy/private/TechnoRunner
git checkout -b qt-rewrite
```

- [ ] **Step 2: Write `vcpkg.json`**

```json
{
  "name": "technorunner",
  "version-string": "1.0.0",
  "dependencies": [ "libarchive" ]
}
```

- [ ] **Step 3: Write `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.21)
project(TechnoRunner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Test)
find_package(LibArchive REQUIRED)

qt_standard_project_setup()

# All library sources (everything under src/ except main.cpp) are added by later
# tasks via target_sources(). Start empty so the lib exists from Task 1.
add_library(tprunner_lib STATIC)
target_include_directories(tprunner_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tprunner_lib PUBLIC Qt6::Widgets Qt6::Network LibArchive::LibArchive)

qt_add_executable(TechnoRunner src/main.cpp)
target_link_libraries(TechnoRunner PRIVATE tprunner_lib)
set_target_properties(TechnoRunner PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)

# ---- Tests ----
enable_testing()
function(tprunner_add_test name)
    qt_add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE tprunner_lib Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

tprunner_add_test(SmokeTest tests/SmokeTest.cpp)
```

- [ ] **Step 4: Write `src/main.cpp` (minimal placeholder)**

```cpp
#include <QApplication>
#include <QWidget>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QWidget w;
    w.resize(400, 200);
    w.show();
    return app.exec();
}
```

- [ ] **Step 5: Write `tests/SmokeTest.cpp` (failing first)**

```cpp
#include <QtTest>

class SmokeTest : public QObject {
    Q_OBJECT
private slots:
    void arithmeticHolds() { QCOMPARE(2 + 2, 5); } // intentionally wrong first
};

QTEST_APPLESS_MAIN(SmokeTest)
#include "SmokeTest.moc"
```

- [ ] **Step 6: Append build dir ignore to `.gitignore`**

Add these lines to `.gitignore`:

```
/build/
/vcpkg_installed/
```

- [ ] **Step 7: Configure & build, run the test, verify it FAILS**

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build
ctest --test-dir build --output-on-failure
```
Expected: `SmokeTest` FAILS (`Actual: 4 … Expected: 5`). If CMake can't find Qt6, set `-DCMAKE_PREFIX_PATH=<qt6-dir>`. If libarchive isn't found and vcpkg isn't set up, install it: `vcpkg install libarchive` (or `sudo apt install libarchive-dev` for a local Linux dev build) and re-run.

- [ ] **Step 8: Fix the test to pass**

Change `QCOMPARE(2 + 2, 5);` to `QCOMPARE(2 + 2, 4);`.

- [ ] **Step 9: Rebuild, run, verify PASS**

```bash
cmake --build build && ctest --test-dir build --output-on-failure
```
Expected: `SmokeTest` PASSES; `TechnoRunner` binary exists in `build/`.

- [ ] **Step 10: Commit**

```bash
git add CMakeLists.txt vcpkg.json src/main.cpp tests/SmokeTest.cpp .gitignore
git commit -m "build: Qt6 CMake skeleton + CTest harness"
```

---

## Task 2: util/Hash — SHA-256 → Base64

**Files:**
- Create: `src/util/Hash.h`, `src/util/Hash.cpp`, `tests/HashTest.cpp`
- Modify: `CMakeLists.txt` (add source + test)

**Interfaces:**
- Produces: `QString tprunner::sha256Base64(const QString& filePath)` — Base64 SHA-256 of the file; throws `std::runtime_error` if the file can't be opened.

- [ ] **Step 1: Write the failing test**

```cpp
#include <QtTest>
#include <QTemporaryFile>
#include "util/Hash.h"

class HashTest : public QObject {
    Q_OBJECT
private slots:
    void emptyFileHasKnownHash() {
        QTemporaryFile f; QVERIFY(f.open()); f.close();
        // SHA-256 of empty input, Base64-encoded (matches Java Base64.getEncoder()):
        QCOMPARE(tprunner::sha256Base64(f.fileName()),
                 QStringLiteral("47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU="));
    }
    void knownContentHash() {
        QTemporaryFile f; QVERIFY(f.open()); f.write("abc"); f.close();
        // SHA-256("abc") Base64:
        QCOMPARE(tprunner::sha256Base64(f.fileName()),
                 QStringLiteral("ungWv48Bz+pBQUDeXa4iI7ADYaOWF3qctBD/YfIAFa0="));
    }
    void missingFileThrows() {
        QVERIFY_EXCEPTION_THROWN(tprunner::sha256Base64("/no/such/file"), std::runtime_error);
    }
};

QTEST_APPLESS_MAIN(HashTest)
#include "HashTest.moc"
```

- [ ] **Step 2: Add to CMake and run to verify FAIL (won't compile yet)**

In `CMakeLists.txt`, after the `add_library(tprunner_lib STATIC)` line add:
```cmake
target_sources(tprunner_lib PRIVATE src/util/Hash.cpp)
```
And after the SmokeTest line add:
```cmake
tprunner_add_test(HashTest tests/HashTest.cpp)
```
Run:
```bash
cmake -B build -S . && cmake --build build 2>&1 | tail -5
```
Expected: compile error (`Hash.h` not found).

- [ ] **Step 3: Write `src/util/Hash.h`**

```cpp
#pragma once
#include <QString>

namespace tprunner {
QString sha256Base64(const QString& filePath);
}
```

- [ ] **Step 4: Write `src/util/Hash.cpp`**

```cpp
#include "util/Hash.h"
#include <QFile>
#include <QCryptographicHash>
#include <stdexcept>

namespace tprunner {

QString sha256Base64(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error(("cannot open file: " + filePath).toStdString());

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
        throw std::runtime_error(("cannot read file: " + filePath).toStdString());

    return QString::fromLatin1(hash.result().toBase64());
}

}
```

- [ ] **Step 5: Build & run, verify PASS**

```bash
cmake --build build && ctest --test-dir build -R HashTest --output-on-failure
```
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/util/Hash.* tests/HashTest.cpp
git commit -m "feat: SHA-256 Base64 hashing (Hash)"
```

---

## Task 3: util/Platform — OS/arch detection & matching

**Files:**
- Create: `src/util/Platform.h`, `src/util/Platform.cpp`, `tests/PlatformTest.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: nothing.
- Produces: `Os`, `CpuArch` enums; `osFromString`, `archFromString`, `currentOs`, `currentArch`, `matchesCurrent`, `workingDirectoryForOs` (used by Paths, JavaDownloader, Launcher).

- [ ] **Step 1: Write the failing test**

```cpp
#include <QtTest>
#include "util/Platform.h"

using namespace tprunner;

class PlatformTest : public QObject {
    Q_OBJECT
private slots:
    void osFromStringMatchesOslib() {
        QCOMPARE(osFromString("Linux"),   Os::Linux);
        QCOMPARE(osFromString("linux"),   Os::Linux);
        QCOMPARE(osFromString("Windows"), Os::Windows);
        QCOMPARE(osFromString("macOS"),   Os::MacOs);
        QCOMPARE(osFromString("mac os x"),Os::MacOs);   // contains "mac"
    }
    void archFromStringMatchesOslib() {
        QCOMPARE(archFromString("x86_64"), CpuArch::X86_64);
        QCOMPARE(archFromString("amd64"),  CpuArch::X86_64);
        QCOMPARE(archFromString("arm"),    CpuArch::Arm);   // matches "ARM" case-insensitively
        QCOMPARE(archFromString("arm64"),  CpuArch::Arm);
        QCOMPARE(archFromString("i686"),   CpuArch::X86);
        QCOMPARE(archFromString("aarch64"),CpuArch::Unknown); // not in any oslib list
    }
    void workingDirectoryPerOs() {
        QCOMPARE(workingDirectoryForOs(Os::Linux, "/home/u", "/ignored"),
                 QStringLiteral("/home/u/.minecraft"));
        QCOMPARE(workingDirectoryForOs(Os::MacOs, "/Users/u", "/ignored"),
                 QStringLiteral("/Users/u/Library/Application Support/minecraft"));
        QCOMPARE(workingDirectoryForOs(Os::Windows, "C:/Users/u", "C:/Users/u/AppData/Roaming"),
                 QStringLiteral("C:/Users/u/AppData/Roaming/.minecraft"));
        QCOMPARE(workingDirectoryForOs(Os::Windows, "C:/Users/u", ""),  // APPDATA unset → home
                 QStringLiteral("C:/Users/u/.minecraft"));
    }
    void currentOsIsKnown() {
        QVERIFY(currentOs() != Os::Unknown);
    }
};

QTEST_APPLESS_MAIN(PlatformTest)
#include "PlatformTest.moc"
```

- [ ] **Step 2: Add to CMake and run to verify FAIL**

Add to `CMakeLists.txt`:
```cmake
target_sources(tprunner_lib PRIVATE src/util/Platform.cpp)
```
```cmake
tprunner_add_test(PlatformTest tests/PlatformTest.cpp)
```
Run: `cmake -B build -S . && cmake --build build 2>&1 | tail -5` → compile error.

- [ ] **Step 3: Write `src/util/Platform.h`**

```cpp
#pragma once
#include <QString>

namespace tprunner {

enum class Os { Windows, MacOs, Linux, Solaris, Bsd, Unknown };
enum class CpuArch { X86, X86_64, Arm, Unknown };

Os osFromString(const QString& s);          // oslib OperatingSystem.getOperatingSystem(String)
CpuArch archFromString(const QString& s);   // oslib Arch.getArch(String)

Os currentOs();
CpuArch currentArch();                       // incl. macOS Rosetta logic

bool matchesCurrent(const QString& entryType, const QString& entryArch);

QString workingDirectoryForOs(Os os, const QString& home, const QString& appData);

}
```

- [ ] **Step 4: Write `src/util/Platform.cpp`**

```cpp
#include "util/Platform.h"
#include <QSysInfo>
#include <QProcessEnvironment>

#ifdef Q_OS_MACOS
#include <sys/sysctl.h>
#include <sys/utsname.h>
#endif

namespace tprunner {

namespace {
struct OsSpec { Os os; const char* name; QStringList search; };
const QList<OsSpec>& osSpecs() {
    static const QList<OsSpec> specs = {
        { Os::Windows, "Windows", {"win"} },
        { Os::MacOs,   "macOS",   {"mac"} },
        { Os::Linux,   "Linux",   {"linux"} },
        { Os::Solaris, "Solaris", {"solaris","sunos"} },
        { Os::Bsd,     "BSD",     {} },
        { Os::Unknown, "unknown", {"unknown"} },
    };
    return specs;
}
struct ArchSpec { CpuArch arch; QStringList search; };
const QList<ArchSpec>& archSpecs() {
    static const QList<ArchSpec> specs = {
        { CpuArch::X86,    {"x86","i386","i486","i586","i686"} },
        { CpuArch::X86_64, {"x86_64","amd64","k8"} },
        { CpuArch::Arm,    {"ARM","arm64"} },
        { CpuArch::Unknown,{"Unknown"} },
    };
    return specs;
}
} // namespace

Os osFromString(const QString& raw) {
    const QString s = raw.toLower();
    for (const auto& spec : osSpecs()) {
        if (QString::fromLatin1(spec.name).compare(s, Qt::CaseInsensitive) == 0)
            return spec.os;
        for (const QString& needle : spec.search)
            if (s.contains(needle.toLower()))
                return spec.os;
    }
    return Os::Unknown;
}

CpuArch archFromString(const QString& s) {
    for (const auto& spec : archSpecs())
        for (const QString& needle : spec.search)
            if (s.compare(needle, Qt::CaseInsensitive) == 0)
                return spec.arch;
    return CpuArch::Unknown;
}

Os currentOs() {
#if defined(Q_OS_WIN)
    return Os::Windows;
#elif defined(Q_OS_MACOS)
    return Os::MacOs;
#elif defined(Q_OS_LINUX)
    return Os::Linux;
#else
    return Os::Unknown;
#endif
}

CpuArch currentArch() {
#if defined(Q_OS_MACOS)
    int translated = 0; size_t size = sizeof(translated);
    if (sysctlbyname("sysctl.proc_translated", &translated, &size, nullptr, 0) == 0 && translated == 1)
        return CpuArch::Arm;                       // running under Rosetta → arm machine
    struct utsname u{};
    if (uname(&u) == 0)
        return archFromString(QString::fromLatin1(u.machine)); // "arm64" / "x86_64"
    return archFromString(QSysInfo::currentCpuArchitecture());
#elif defined(Q_OS_WIN)
    if (QProcessEnvironment::systemEnvironment().contains("ProgramFiles(x86)"))
        return CpuArch::X86_64;
    return archFromString(QSysInfo::currentCpuArchitecture());
#else
    return archFromString(QSysInfo::currentCpuArchitecture()); // "x86_64", "arm64", ...
#endif
}

bool matchesCurrent(const QString& entryType, const QString& entryArch) {
    return osFromString(entryType) == currentOs()
        && archFromString(entryArch) == currentArch();
}

QString workingDirectoryForOs(Os os, const QString& home, const QString& appData) {
    switch (os) {
        case Os::Windows:
            return (appData.isEmpty() ? home : appData) + "/.minecraft";
        case Os::MacOs:
            return home + "/Library/Application Support/minecraft";
        default: // Linux/Solaris/Bsd/Unknown → ~/.minecraft (matches mclauncher-api)
            return home + "/.minecraft";
    }
}

}
```

- [ ] **Step 5: Build & run, verify PASS**

```bash
cmake --build build && ctest --test-dir build -R PlatformTest --output-on-failure
```
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/util/Platform.* tests/PlatformTest.cpp
git commit -m "feat: OS/arch detection and jres matching (Platform)"
```

---

## Task 4: util/Paths — directory layout

**Files:**
- Create: `src/util/Paths.h`, `src/util/Paths.cpp`, `tests/PathsTest.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `Os`, `osFromString`, `currentOs`, `workingDirectoryForOs` (Task 3).
- Produces: `Paths::*` accessors (used by JavaDownloader, LauncherDownloader, Launcher, main).

- [ ] **Step 1: Write the failing test**

```cpp
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "util/Paths.h"

using namespace tprunner;

class PathsTest : public QObject {
    Q_OBJECT
    QTemporaryDir tmp_;
private slots:
    void init()    { Paths::setBaseOverride(tmp_.path()); }
    void cleanup() { Paths::setBaseOverride(QString()); }

    void subPathsLayout() {
        QCOMPARE(Paths::baseDirectory(), tmp_.path());
        QCOMPARE(Paths::temporaryDirectory(), tmp_.path() + "/tmp");
        QCOMPARE(Paths::javaDirectory(),      tmp_.path() + "/jre");
        QCOMPARE(Paths::jrePathFile(),        tmp_.path() + "/jrepath.txt");
        QCOMPARE(Paths::launcherFile(),       tmp_.path() + "/launcher.jar");
        QVERIFY(QFileInfo::exists(tmp_.path() + "/tmp")); // created on demand
    }
    void jrePathFallsBackToJava() {
        QCOMPARE(Paths::jrePath(), QStringLiteral("java")); // no jrepath.txt yet
    }
    void jrePathReadsWrittenValueWhenExists() {
        const QString fakeJava = tmp_.path() + "/fakejava";
        { QFile f(fakeJava); QVERIFY(f.open(QIODevice::WriteOnly)); }
        Paths::writeJrePath(fakeJava);
        QCOMPARE(Paths::jrePath(), fakeJava);
    }
    void jrePathFallsBackWhenTargetMissing() {
        Paths::writeJrePath(tmp_.path() + "/does-not-exist");
        QCOMPARE(Paths::jrePath(), QStringLiteral("java"));
    }
};

QTEST_APPLESS_MAIN(PathsTest)
#include "PathsTest.moc"
```

- [ ] **Step 2: Add to CMake and run to verify FAIL**

```cmake
target_sources(tprunner_lib PRIVATE src/util/Paths.cpp)
```
```cmake
tprunner_add_test(PathsTest tests/PathsTest.cpp)
```
Run build → compile error.

- [ ] **Step 3: Write `src/util/Paths.h`**

```cpp
#pragma once
#include <QString>

namespace tprunner {

class Paths {
public:
    static QString workingDirectory();
    static QString baseDirectory();
    static QString temporaryDirectory();
    static QString javaDirectory();
    static QString jrePathFile();
    static void    writeJrePath(const QString& absoluteJavaPath);
    static QString jrePath();
    static QString launcherFile();
    static QString launcherOutLog();
    static QString launcherErrLog();
    static QString runnerLog();

    static void setBaseOverride(const QString& dir); // tests only; "" clears
};

}
```

- [ ] **Step 4: Write `src/util/Paths.cpp`**

```cpp
#include "util/Paths.h"
#include "util/Platform.h"
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>

namespace tprunner {

namespace { QString g_baseOverride; }

void Paths::setBaseOverride(const QString& dir) { g_baseOverride = dir; }

QString Paths::workingDirectory() {
    const auto env = QProcessEnvironment::systemEnvironment();
    return workingDirectoryForOs(currentOs(), QDir::homePath(), env.value("APPDATA"));
}

QString Paths::baseDirectory() {
    const QString base = g_baseOverride.isEmpty()
        ? workingDirectory() + "/technomine"
        : g_baseOverride;
    QDir().mkpath(base);
    return base;
}

QString Paths::temporaryDirectory() { const QString d = baseDirectory() + "/tmp"; QDir().mkpath(d); return d; }
QString Paths::javaDirectory()      { const QString d = baseDirectory() + "/jre"; QDir().mkpath(d); return d; }
QString Paths::jrePathFile()        { return baseDirectory() + "/jrepath.txt"; }
QString Paths::launcherFile()       { return baseDirectory() + "/launcher.jar"; }
QString Paths::launcherOutLog()     { return baseDirectory() + "/launcherout.log"; }
QString Paths::launcherErrLog()     { return baseDirectory() + "/launchererr.log"; }
QString Paths::runnerLog()          { return baseDirectory() + "/runner.log"; }

void Paths::writeJrePath(const QString& absoluteJavaPath) {
    QFile f(jrePathFile());
    if (f.exists()) f.remove();
    if (f.open(QIODevice::WriteOnly)) f.write(absoluteJavaPath.toUtf8());
}

QString Paths::jrePath() {
    QString javaPath;
    QFile f(jrePathFile());
    if (f.exists() && f.open(QIODevice::ReadOnly))
        javaPath = QString::fromUtf8(f.readAll());
    if (javaPath.isEmpty() || !QFileInfo::exists(javaPath))
        javaPath = "java";
    return javaPath;
}

}
```

- [ ] **Step 5: Build & run, verify PASS**

```bash
cmake --build build && ctest --test-dir build -R PathsTest --output-on-failure
```

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/util/Paths.* tests/PathsTest.cpp
git commit -m "feat: technomine directory layout (Paths)"
```

---

## Task 5: models — JSON parsing

**Files:**
- Create: `src/models/Models.h`, `src/models/Models.cpp`, `tests/ModelsTest.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `JavaBinaryModel`, `LauncherModel` + parsers (used by JavaDownloader, LauncherDownloader).

- [ ] **Step 1: Write the failing test**

```cpp
#include <QtTest>
#include "models/Models.h"

using namespace tprunner;

class ModelsTest : public QObject {
    Q_OBJECT
private slots:
    void parsesJresArray() {
        const QByteArray json = R"([
          {"type":"Linux","arch":"x86_64","extension":"tar.gz",
           "downloadUrl":"https://x/l.tar.gz","javaRelativePath":"jre/bin/java"},
          {"type":"macOS","arch":"arm","extension":"zip",
           "downloadUrl":"https://x/m.zip","javaRelativePath":"jre.jre/bin/java"}
        ])";
        const auto list = JavaBinaryModel::listFromJson(json);
        QCOMPARE(list.size(), 2);
        QCOMPARE(list[0].type, QStringLiteral("Linux"));
        QCOMPARE(list[0].extension, QStringLiteral("tar.gz"));
        QCOMPARE(list[1].arch, QStringLiteral("arm"));
        QCOMPARE(list[1].javaRelativePath, QStringLiteral("jre.jre/bin/java"));
    }
    void parsesLauncherObjectIncludingDashedKey() {
        const QByteArray json = R"({
          "version":"1.0.15",
          "downloadFullPath":"https://minecraft.glitchless.ru/1.0.15.jar",
          "SHA-256":"K6qafxioI7LGPcOeE8ZZrljmGenIfg860yPvjPV6JeM="
        })";
        const auto m = LauncherModel::fromJson(json);
        QVERIFY(m.has_value());
        QCOMPARE(m->version, QStringLiteral("1.0.15"));
        QCOMPARE(m->downloadUrl, QStringLiteral("https://minecraft.glitchless.ru/1.0.15.jar"));
        QCOMPARE(m->sha256, QStringLiteral("K6qafxioI7LGPcOeE8ZZrljmGenIfg860yPvjPV6JeM="));
    }
    void invalidLauncherJsonReturnsNullopt() {
        QVERIFY(!LauncherModel::fromJson("not json").has_value());
    }
};

QTEST_APPLESS_MAIN(ModelsTest)
#include "ModelsTest.moc"
```

- [ ] **Step 2: Add to CMake and verify FAIL**

```cmake
target_sources(tprunner_lib PRIVATE src/models/Models.cpp)
```
```cmake
tprunner_add_test(ModelsTest tests/ModelsTest.cpp)
```

- [ ] **Step 3: Write `src/models/Models.h`**

```cpp
#pragma once
#include <QString>
#include <QList>
#include <QByteArray>
#include <optional>

namespace tprunner {

struct JavaBinaryModel {
    QString type, arch, downloadUrl, javaRelativePath, extension;
    static QList<JavaBinaryModel> listFromJson(const QByteArray& json);
};

struct LauncherModel {
    QString version, downloadUrl, sha256;
    static std::optional<LauncherModel> fromJson(const QByteArray& json);
};

}
```

- [ ] **Step 4: Write `src/models/Models.cpp`**

```cpp
#include "models/Models.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

namespace tprunner {

QList<JavaBinaryModel> JavaBinaryModel::listFromJson(const QByteArray& json) {
    QList<JavaBinaryModel> out;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) return out;
    for (const QJsonValue& v : doc.array()) {
        const QJsonObject o = v.toObject();
        JavaBinaryModel m;
        m.type             = o.value("type").toString();
        m.arch             = o.value("arch").toString();
        m.downloadUrl      = o.value("downloadUrl").toString();
        m.javaRelativePath = o.value("javaRelativePath").toString();
        m.extension        = o.value("extension").toString();
        out.append(m);
    }
    return out;
}

std::optional<LauncherModel> LauncherModel::fromJson(const QByteArray& json) {
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) return std::nullopt;
    const QJsonObject o = doc.object();
    LauncherModel m;
    m.version     = o.value("version").toString();
    m.downloadUrl = o.value("downloadFullPath").toString();
    m.sha256      = o.value("SHA-256").toString();
    return m;
}

}
```

- [ ] **Step 5: Build & run, verify PASS**

```bash
cmake --build build && ctest --test-dir build -R ModelsTest --output-on-failure
```

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/models/Models.* tests/ModelsTest.cpp
git commit -m "feat: jres.json/launcher.json models"
```

---

## Task 6: util/Archive — libarchive extraction

**Files:**
- Create: `src/util/Archive.h`, `src/util/Archive.cpp`, `tests/ArchiveTest.cpp`, `tests/fixtures/` (generated)
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `Archive::extractZip`, `Archive::extractTarGz` (used by JavaDownloader). Both throw `std::runtime_error` on failure.

- [ ] **Step 1: Generate tiny fixture archives**

```bash
mkdir -p tests/fixtures/payload tests/fixtures/out
printf 'hello-tar' > tests/fixtures/payload/a.txt
printf 'nested'    > tests/fixtures/payload/sub_b.txt   # placeholder; recreated below with dir
rm -f tests/fixtures/payload/sub_b.txt
mkdir -p tests/fixtures/payload/sub
printf 'nested' > tests/fixtures/payload/sub/b.txt
( cd tests/fixtures/payload && tar -czf ../sample.tar.gz a.txt sub/b.txt )
( cd tests/fixtures/payload && zip -q -r ../sample.zip a.txt sub/b.txt )
rm -rf tests/fixtures/payload tests/fixtures/out
ls -l tests/fixtures
```
Expected: `tests/fixtures/sample.tar.gz` and `tests/fixtures/sample.zip` exist.

- [ ] **Step 2: Write the failing test**

```cpp
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "util/Archive.h"

using namespace tprunner;

namespace {
QString fixture(const QString& name) {
    return QString(SRCDIR) + "/fixtures/" + name; // SRCDIR injected by CMake
}
QString readAll(const QString& path) {
    QFile f(path); f.open(QIODevice::ReadOnly); return QString::fromUtf8(f.readAll());
}
}

class ArchiveTest : public QObject {
    Q_OBJECT
private slots:
    void extractsTarGz() {
        QTemporaryDir dir;
        Archive::extractTarGz(fixture("sample.tar.gz"), dir.path());
        QCOMPARE(readAll(dir.path() + "/a.txt"), QStringLiteral("hello-tar"));
        QCOMPARE(readAll(dir.path() + "/sub/b.txt"), QStringLiteral("nested"));
    }
    void extractsZip() {
        QTemporaryDir dir;
        Archive::extractZip(fixture("sample.zip"), dir.path());
        QCOMPARE(readAll(dir.path() + "/a.txt"), QStringLiteral("hello-tar"));
        QCOMPARE(readAll(dir.path() + "/sub/b.txt"), QStringLiteral("nested"));
    }
    void missingArchiveThrows() {
        QTemporaryDir dir;
        QVERIFY_EXCEPTION_THROWN(Archive::extractZip("/no/such.zip", dir.path()), std::runtime_error);
    }
};

QTEST_APPLESS_MAIN(ArchiveTest)
#include "ArchiveTest.moc"
```

- [ ] **Step 3: Add to CMake (with SRCDIR define) and verify FAIL**

```cmake
target_sources(tprunner_lib PRIVATE src/util/Archive.cpp)
```
```cmake
tprunner_add_test(ArchiveTest tests/ArchiveTest.cpp)
target_compile_definitions(ArchiveTest PRIVATE SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/tests")
```

- [ ] **Step 4: Write `src/util/Archive.h`**

```cpp
#pragma once
#include <QString>

namespace tprunner {

class Archive {
public:
    static void extractZip(const QString& archivePath, const QString& destDir);
    static void extractTarGz(const QString& archivePath, const QString& destDir);
};

}
```

- [ ] **Step 5: Write `src/util/Archive.cpp`**

libarchive auto-detects format+filter, so both methods delegate to one extractor.

```cpp
#include "util/Archive.h"
#include <QDir>
#include <QByteArray>
#include <archive.h>
#include <archive_entry.h>
#include <stdexcept>

namespace tprunner {

namespace {

void copyData(struct archive* in, struct archive* out) {
    const void* buff; size_t size; la_int64_t offset;
    for (;;) {
        int r = archive_read_data_block(in, &buff, &size, &offset);
        if (r == ARCHIVE_EOF) return;
        if (r < ARCHIVE_OK) throw std::runtime_error(archive_error_string(in));
        if (archive_write_data_block(out, buff, size, offset) < ARCHIVE_OK)
            throw std::runtime_error(archive_error_string(out));
    }
}

void extractTo(const QString& archivePath, const QString& destDir) {
    QDir().mkpath(destDir);
    struct archive* a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    struct archive* ext = archive_write_disk_new();
    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
        ARCHIVE_EXTRACT_ACL  | ARCHIVE_EXTRACT_FFLAGS);
    archive_write_disk_set_standard_lookup(ext);

    const QByteArray src = archivePath.toUtf8();
    if (archive_read_open_filename(a, src.constData(), 10240) != ARCHIVE_OK) {
        const std::string err = archive_error_string(a) ? archive_error_string(a) : "open failed";
        archive_read_free(a); archive_write_free(ext);
        throw std::runtime_error(err);
    }

    const QByteArray destPrefix = (destDir + "/").toUtf8();
    try {
        struct archive_entry* entry;
        int r;
        while ((r = archive_read_next_header(a, &entry)) != ARCHIVE_EOF) {
            if (r < ARCHIVE_OK) throw std::runtime_error(archive_error_string(a));
            const QByteArray full = destPrefix + archive_entry_pathname(entry);
            archive_entry_set_pathname(entry, full.constData());
            if (archive_write_header(ext, entry) < ARCHIVE_OK)
                throw std::runtime_error(archive_error_string(ext));
            if (archive_entry_size(entry) > 0)
                copyData(a, ext);
            archive_write_finish_entry(ext);
        }
    } catch (...) {
        archive_read_free(a); archive_write_free(ext);
        throw;
    }
    archive_read_free(a);
    archive_write_free(ext);
}

} // namespace

void Archive::extractZip(const QString& archivePath, const QString& destDir)   { extractTo(archivePath, destDir); }
void Archive::extractTarGz(const QString& archivePath, const QString& destDir) { extractTo(archivePath, destDir); }

}
```

- [ ] **Step 6: Build & run, verify PASS**

```bash
cmake -B build -S . && cmake --build build && ctest --test-dir build -R ArchiveTest --output-on-failure
```

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt src/util/Archive.* tests/ArchiveTest.cpp tests/fixtures/sample.tar.gz tests/fixtures/sample.zip
git commit -m "feat: libarchive zip/tar.gz extraction (Archive)"
```

---

## Task 7: net/Downloader + ProgressMonitor interface

**Files:**
- Create: `src/app/ProgressMonitor.h`, `src/net/Downloader.h`, `src/net/Downloader.cpp`, `tests/DownloaderTest.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: nothing (ProgressMonitor defined here).
- Produces: `ProgressMonitor` (interface used by every download-y task), `Downloader::httpGet`, `Downloader::downloadToFile`.

- [ ] **Step 1: Write `src/app/ProgressMonitor.h`**

```cpp
#pragma once
#include <QString>

namespace tprunner {

class ProgressMonitor {
public:
    virtual ~ProgressMonitor() = default;
    virtual void setProgress(int progress) = 0;
    virtual void setMax(int max) = 0;
    virtual void incrementProgress(int amount) = 0;
    virtual void setStatus(const QString& status) = 0;
};

}
```

- [ ] **Step 2: Write the failing test** (uses a local `QTcpServer` HTTP stub + a recording monitor)

```cpp
#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QNetworkAccessManager>
#include <QFile>
#include "net/Downloader.h"
#include "app/ProgressMonitor.h"

using namespace tprunner;

namespace {
class RecordingMonitor : public ProgressMonitor {
public:
    int lastProgress = -1, lastMax = -1; QString lastStatus;
    void setProgress(int p) override { lastProgress = p; }
    void setMax(int m) override { lastMax = m; }
    void incrementProgress(int a) override { lastProgress += a; }
    void setStatus(const QString& s) override { lastStatus = s; }
};

// Minimal one-shot HTTP/1.1 server returning a fixed body.
class StubServer : public QObject {
    Q_OBJECT
public:
    QByteArray body;
    quint16 port() const { return server_.serverPort(); }
    bool start() {
        if (!server_.listen(QHostAddress::LocalHost)) return false;
        connect(&server_, &QTcpServer::newConnection, this, [this] {
            QTcpSocket* s = server_.nextPendingConnection();
            connect(s, &QTcpSocket::readyRead, this, [this, s] {
                s->readAll();
                QByteArray resp = "HTTP/1.1 200 OK\r\nContent-Length: "
                    + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
                s->write(resp); s->flush(); s->disconnectFromHost();
            });
        });
        return true;
    }
private:
    QTcpServer server_;
};
}

class DownloaderTest : public QObject {
    Q_OBJECT
private slots:
    void httpGetReturnsBody() {
        StubServer srv; srv.body = "hello-body"; QVERIFY(srv.start());
        QNetworkAccessManager nam; Downloader dl(&nam);
        const QByteArray out = dl.httpGet(QString("http://127.0.0.1:%1/x").arg(srv.port()));
        QCOMPARE(out, QByteArray("hello-body"));
    }
    void downloadToFileWritesAndReportsProgress() {
        StubServer srv; srv.body = QByteArray(2048, 'z'); QVERIFY(srv.start());
        QNetworkAccessManager nam; Downloader dl(&nam);
        QTemporaryDir dir; const QString dest = dir.path() + "/out.bin";
        RecordingMonitor mon;
        dl.downloadToFile(QString("http://127.0.0.1:%1/x").arg(srv.port()), dest, &mon);
        QFile f(dest); QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll().size(), 2048);
        QCOMPARE(mon.lastMax, 2048);
        QVERIFY(mon.lastProgress > 0);
    }
    void httpGetOnConnectionRefusedThrows() {
        QNetworkAccessManager nam; Downloader dl(&nam);
        QVERIFY_EXCEPTION_THROWN(dl.httpGet("http://127.0.0.1:1/nope"), std::runtime_error);
    }
};

QTEST_MAIN(DownloaderTest)
#include "DownloaderTest.moc"
```

- [ ] **Step 3: Add to CMake and verify FAIL**

```cmake
target_sources(tprunner_lib PRIVATE src/net/Downloader.cpp)
```
```cmake
tprunner_add_test(DownloaderTest tests/DownloaderTest.cpp)
```

- [ ] **Step 4: Write `src/net/Downloader.h`**

```cpp
#pragma once
#include <QString>
#include <QByteArray>

class QNetworkAccessManager;

namespace tprunner {

class ProgressMonitor;

class Downloader {
public:
    explicit Downloader(QNetworkAccessManager* nam);
    QByteArray httpGet(const QString& url);
    void downloadToFile(const QString& url, const QString& destPath, ProgressMonitor* monitor);
private:
    QNetworkAccessManager* nam_;
};

}
```

- [ ] **Step 5: Write `src/net/Downloader.cpp`**

```cpp
#include "net/Downloader.h"
#include "app/ProgressMonitor.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QUrl>
#include <stdexcept>

namespace tprunner {

Downloader::Downloader(QNetworkAccessManager* nam) : nam_(nam) {}

namespace {
QNetworkRequest makeRequest(const QString& url) {
    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    return req;
}
}

QByteArray Downloader::httpGet(const QString& url) {
    QNetworkReply* reply = nam_->get(makeRequest(url));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const QNetworkReply::NetworkError err = reply->error();
    const QByteArray data = reply->readAll();
    reply->deleteLater();
    if (err != QNetworkReply::NoError)
        throw std::runtime_error(("GET failed: " + url).toStdString());
    return data;
}

void Downloader::downloadToFile(const QString& url, const QString& destPath, ProgressMonitor* monitor) {
    QFile file(destPath);
    if (!file.open(QIODevice::WriteOnly))
        throw std::runtime_error(("cannot open for write: " + destPath).toStdString());

    QNetworkReply* reply = nam_->get(makeRequest(url));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::readyRead, reply, [&] {
        file.write(reply->readAll());
    });
    if (monitor) {
        QObject::connect(reply, &QNetworkReply::downloadProgress, reply,
            [monitor](qint64 received, qint64 total) {
                if (total > 0) { monitor->setMax(int(total)); monitor->setProgress(int(received)); }
            });
    }
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    file.write(reply->readAll());
    file.flush();
    file.close();
    const QNetworkReply::NetworkError err = reply->error();
    reply->deleteLater();
    if (err != QNetworkReply::NoError) {
        QFile::remove(destPath);
        throw std::runtime_error(("download failed: " + url).toStdString());
    }
}

}
```

- [ ] **Step 6: Build & run, verify PASS**

```bash
cmake -B build -S . && cmake --build build && ctest --test-dir build -R DownloaderTest --output-on-failure
```

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt src/app/ProgressMonitor.h src/net/Downloader.* tests/DownloaderTest.cpp
git commit -m "feat: sync HTTP downloader with progress (Downloader)"
```

---

## Task 8: net/JavaDownloader

**Files:**
- Create: `src/net/JavaDownloader.h`, `src/net/JavaDownloader.cpp`, `tests/JavaDownloaderTest.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `Downloader` (Task 7), `JavaBinaryModel` (Task 5), `Os`/`CpuArch`/`archFromString`/`osFromString`/`currentOs`/`currentArch` (Task 3), `Archive` (Task 6), `Paths` (Task 4), `ProgressMonitor` (Task 7).
- Produces: `JavaDownloader` + static `findMatch` (used by Bootstrapper).

- [ ] **Step 1: Write the failing test** (focus on the pure `findMatch`)

```cpp
#include <QtTest>
#include "net/JavaDownloader.h"
#include "util/Platform.h"

using namespace tprunner;

class JavaDownloaderTest : public QObject {
    Q_OBJECT
    QList<JavaBinaryModel> list_;
private slots:
    void initTestCase() {
        list_ = JavaBinaryModel::listFromJson(R"([
          {"type":"Linux","arch":"x86_64","extension":"tar.gz","downloadUrl":"u1","javaRelativePath":"p1"},
          {"type":"macOS","arch":"x86_64","extension":"zip","downloadUrl":"u2","javaRelativePath":"p2"},
          {"type":"macOS","arch":"arm","extension":"zip","downloadUrl":"u3","javaRelativePath":"p3"}
        ])");
    }
    void matchesLinuxX64() {
        const auto m = JavaDownloader::findMatch(list_, Os::Linux, CpuArch::X86_64);
        QVERIFY(m.has_value()); QCOMPARE(m->downloadUrl, QStringLiteral("u1"));
    }
    void matchesMacArmDistinctFromMacX64() {
        const auto m = JavaDownloader::findMatch(list_, Os::MacOs, CpuArch::Arm);
        QVERIFY(m.has_value()); QCOMPARE(m->javaRelativePath, QStringLiteral("p3"));
    }
    void noMatchReturnsNullopt() {
        const auto m = JavaDownloader::findMatch(list_, Os::Windows, CpuArch::X86_64);
        QVERIFY(!m.has_value());
    }
};

QTEST_APPLESS_MAIN(JavaDownloaderTest)
#include "JavaDownloaderTest.moc"
```

- [ ] **Step 2: Add to CMake and verify FAIL**

```cmake
target_sources(tprunner_lib PRIVATE src/net/JavaDownloader.cpp)
```
```cmake
tprunner_add_test(JavaDownloaderTest tests/JavaDownloaderTest.cpp)
```

- [ ] **Step 3: Write `src/net/JavaDownloader.h`**

```cpp
#pragma once
#include <QString>
#include <optional>
#include "models/Models.h"

namespace tprunner {

class Downloader;
class ProgressMonitor;
enum class Os;
enum class CpuArch;

class JavaDownloader {
public:
    explicit JavaDownloader(Downloader* dl);
    void init();
    bool hasMatch() const;
    QString download(ProgressMonitor* monitor);   // returns absolute java path, or "" if no match
    static std::optional<JavaBinaryModel> findMatch(const QList<JavaBinaryModel>& list,
                                                    Os os, CpuArch arch);
private:
    Downloader* dl_;
    std::optional<JavaBinaryModel> selected_;
};

}
```

- [ ] **Step 4: Write `src/net/JavaDownloader.cpp`**

```cpp
#include "net/JavaDownloader.h"
#include "net/Downloader.h"
#include "app/ProgressMonitor.h"
#include "util/Platform.h"
#include "util/Paths.h"
#include "util/Archive.h"
#include <QDir>
#include <QObject>

namespace tprunner {

namespace { const char* kJresUrl = "https://minecraft.glitchless.ru/jres.json"; }

JavaDownloader::JavaDownloader(Downloader* dl) : dl_(dl) {}

std::optional<JavaBinaryModel> JavaDownloader::findMatch(
        const QList<JavaBinaryModel>& list, Os os, CpuArch arch) {
    for (const auto& m : list)
        if (osFromString(m.type) == os && archFromString(m.arch) == arch)
            return m;
    return std::nullopt;
}

void JavaDownloader::init() {
    const QByteArray json = dl_->httpGet(kJresUrl);
    const auto list = JavaBinaryModel::listFromJson(json);
    selected_ = findMatch(list, currentOs(), currentArch());
}

bool JavaDownloader::hasMatch() const { return selected_.has_value(); }

QString JavaDownloader::download(ProgressMonitor* monitor) {
    if (!selected_) return QString();
    const JavaBinaryModel& jb = *selected_;

    const QString jreArchive = Paths::temporaryDirectory() + "/jre." + jb.extension;
    if (monitor) monitor->setStatus(QObject::tr("Загрузка Java..."));
    dl_->downloadToFile(jb.downloadUrl, jreArchive, monitor);
    if (monitor) monitor->setProgress(100);

    if (jb.extension.compare("zip", Qt::CaseInsensitive) == 0)
        Archive::extractZip(jreArchive, Paths::javaDirectory());
    else
        Archive::extractTarGz(jreArchive, Paths::javaDirectory());

    return QDir(Paths::javaDirectory()).filePath(jb.javaRelativePath);
}

}
```

- [ ] **Step 5: Build & run, verify PASS**

```bash
cmake --build build && ctest --test-dir build -R JavaDownloaderTest --output-on-failure
```

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/net/JavaDownloader.* tests/JavaDownloaderTest.cpp
git commit -m "feat: JRE download + extraction (JavaDownloader)"
```

---

## Task 9: net/LauncherDownloader

**Files:**
- Create: `src/net/LauncherDownloader.h`, `src/net/LauncherDownloader.cpp`, `tests/LauncherDownloaderTest.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `Downloader` (Task 7), `LauncherModel` (Task 5), `Paths` (Task 4), `sha256Base64` (Task 2), `ProgressMonitor` (Task 7).
- Produces: `LauncherDownloader` (used by Bootstrapper).

To make `checkFile`/`update` testable without network, the implementation reads the manifest through an injectable seam. Use a protected virtual `fetchManifest()` overridden in a test subclass — and verify the SHA-256 comparison logic directly.

- [ ] **Step 1: Write the failing test**

```cpp
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "net/LauncherDownloader.h"
#include "util/Paths.h"
#include "util/Hash.h"

using namespace tprunner;

namespace {
// Subclass that injects a manifest body instead of hitting the network.
class FakeLauncherDownloader : public LauncherDownloader {
public:
    QByteArray manifest;
    explicit FakeLauncherDownloader() : LauncherDownloader(nullptr) {}
protected:
    QByteArray fetchManifest() override { return manifest; }
};
void writeFile(const QString& path, const QByteArray& data) {
    QFile f(path); f.open(QIODevice::WriteOnly); f.write(data); f.close();
}
}

class LauncherDownloaderTest : public QObject {
    Q_OBJECT
    QTemporaryDir tmp_;
private slots:
    void init()    { Paths::setBaseOverride(tmp_.path()); }
    void cleanup() { Paths::setBaseOverride(QString()); }

    void checkFileFalseWhenMissing() {
        FakeLauncherDownloader d; d.manifest = "{}"; d.init();
        QVERIFY(!d.checkFile());
    }
    void checkFileTrueWhenHashMatches() {
        writeFile(Paths::launcherFile(), "JARBYTES");
        const QString h = sha256Base64(Paths::launcherFile());
        FakeLauncherDownloader d;
        d.manifest = QByteArray("{\"version\":\"1\",\"downloadFullPath\":\"u\",\"SHA-256\":\"") + h.toUtf8() + "\"}";
        d.init();
        QVERIFY(d.checkFile());
    }
    void checkFileFalseWhenHashDiffers() {
        writeFile(Paths::launcherFile(), "JARBYTES");
        FakeLauncherDownloader d;
        d.manifest = "{\"version\":\"1\",\"downloadFullPath\":\"u\",\"SHA-256\":\"deadbeef\"}";
        d.init();
        QVERIFY(!d.checkFile());
    }
    void checkFileTrueWhenNoModel() { // file exists, manifest invalid → assume OK (matches original)
        writeFile(Paths::launcherFile(), "JARBYTES");
        FakeLauncherDownloader d; d.manifest = "not json"; d.init();
        QVERIFY(d.checkFile());
    }
};

QTEST_APPLESS_MAIN(LauncherDownloaderTest)
#include "LauncherDownloaderTest.moc"
```

- [ ] **Step 2: Add to CMake and verify FAIL**

```cmake
target_sources(tprunner_lib PRIVATE src/net/LauncherDownloader.cpp)
```
```cmake
tprunner_add_test(LauncherDownloaderTest tests/LauncherDownloaderTest.cpp)
```

- [ ] **Step 3: Write `src/net/LauncherDownloader.h`**

```cpp
#pragma once
#include <QString>
#include <optional>
#include "models/Models.h"

namespace tprunner {

class Downloader;
class ProgressMonitor;

class LauncherDownloader {
public:
    explicit LauncherDownloader(Downloader* dl);
    virtual ~LauncherDownloader() = default;
    void init();
    bool checkFile();
    void update(ProgressMonitor* monitor);
protected:
    virtual QByteArray fetchManifest();   // seam for tests
    Downloader* dl_;
    std::optional<LauncherModel> model_;
};

}
```

- [ ] **Step 4: Write `src/net/LauncherDownloader.cpp`**

```cpp
#include "net/LauncherDownloader.h"
#include "net/Downloader.h"
#include "app/ProgressMonitor.h"
#include "util/Paths.h"
#include "util/Hash.h"
#include <QFile>
#include <QFileInfo>
#include <QObject>

namespace tprunner {

namespace { const char* kLauncherUrl = "https://minecraft.glitchless.ru/launcher.json"; }

LauncherDownloader::LauncherDownloader(Downloader* dl) : dl_(dl) {}

QByteArray LauncherDownloader::fetchManifest() { return dl_->httpGet(kLauncherUrl); }

void LauncherDownloader::init() {
    try { model_ = LauncherModel::fromJson(fetchManifest()); }
    catch (...) { model_ = std::nullopt; }
}

bool LauncherDownloader::checkFile() {
    if (!QFileInfo::exists(Paths::launcherFile())) return false;
    if (!model_) return true;                       // can't verify → assume OK (matches original)
    return sha256Base64(Paths::launcherFile()) == model_->sha256;
}

void LauncherDownloader::update(ProgressMonitor* monitor) {
    if (!model_) return;
    if (monitor) monitor->setStatus(QObject::tr("Скачивание лаунчера..."));
    const QString updateFile = Paths::temporaryDirectory() + "/update_launcher.jar";
    dl_->downloadToFile(model_->downloadUrl, updateFile, monitor);
    if (monitor) monitor->setProgress(100);

    if (sha256Base64(updateFile) != model_->sha256) return;   // verification failed → keep old jar

    QFile launcher(Paths::launcherFile());
    if (!launcher.exists() || launcher.remove())
        QFile::rename(updateFile, Paths::launcherFile());
}

}
```

- [ ] **Step 5: Build & run, verify PASS**

```bash
cmake --build build && ctest --test-dir build -R LauncherDownloaderTest --output-on-failure
```

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/net/LauncherDownloader.* tests/LauncherDownloaderTest.cpp
git commit -m "feat: launcher.jar download/verify/replace (LauncherDownloader)"
```

---

## Task 10: run/Launcher

**Files:**
- Create: `src/run/Launcher.h`, `src/run/Launcher.cpp`, `tests/LauncherTest.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `Os`/`currentOs` (Task 3), `Paths` (Task 4).
- Produces: `Launcher::buildCommand`, `Launcher::run` (used by main).

- [ ] **Step 1: Write the failing test** (pure command builder)

```cpp
#include <QtTest>
#include "run/Launcher.h"
#include "util/Platform.h"

using namespace tprunner;

class LauncherTest : public QObject {
    Q_OBJECT
private slots:
    void buildsUnixShellRedirect() {
        const auto c = Launcher::buildCommand("/jre/bin/java", "/d/launcher.jar",
                                              "/d/out.log", "/d/err.log", Os::Linux);
        QCOMPARE(c.program, QStringLiteral("/bin/sh"));
        QCOMPARE(c.arguments.size(), 2);
        QCOMPARE(c.arguments[0], QStringLiteral("-c"));
        QVERIFY(c.arguments[1].contains("'/jre/bin/java' -jar '/d/launcher.jar'"));
        QVERIFY(c.arguments[1].contains("> '/d/out.log'"));
        QVERIFY(c.arguments[1].contains("2> '/d/err.log'"));
    }
    void buildsWindowsCmdRedirect() {
        const auto c = Launcher::buildCommand("C:/jre/java.exe", "C:/d/launcher.jar",
                                              "C:/d/out.log", "C:/d/err.log", Os::Windows);
        QCOMPARE(c.program, QStringLiteral("cmd.exe"));
        QCOMPARE(c.arguments[0], QStringLiteral("/C"));
        QVERIFY(c.arguments[1].contains("\"C:/jre/java.exe\" -jar \"C:/d/launcher.jar\""));
        QVERIFY(c.arguments[1].contains("> \"C:/d/out.log\""));
        QVERIFY(c.arguments[1].contains("2> \"C:/d/err.log\""));
    }
};

QTEST_APPLESS_MAIN(LauncherTest)
#include "LauncherTest.moc"
```

- [ ] **Step 2: Add to CMake and verify FAIL**

```cmake
target_sources(tprunner_lib PRIVATE src/run/Launcher.cpp)
```
```cmake
tprunner_add_test(LauncherTest tests/LauncherTest.cpp)
```

- [ ] **Step 3: Write `src/run/Launcher.h`**

```cpp
#pragma once
#include <QString>
#include <QStringList>

namespace tprunner {

enum class Os;

struct LaunchCommand { QString program; QStringList arguments; };

class Launcher {
public:
    static LaunchCommand buildCommand(const QString& javaPath, const QString& jarPath,
                                      const QString& outLog, const QString& errLog, Os os);
    static bool run();
};

}
```

- [ ] **Step 4: Write `src/run/Launcher.cpp`**

```cpp
#include "run/Launcher.h"
#include "util/Platform.h"
#include "util/Paths.h"
#include <QProcess>

namespace tprunner {

LaunchCommand Launcher::buildCommand(const QString& javaPath, const QString& jarPath,
                                     const QString& outLog, const QString& errLog, Os os) {
    LaunchCommand c;
    if (os == Os::Windows) {
        c.program = "cmd.exe";
        const QString inner = QString("\"%1\" -jar \"%2\" > \"%3\" 2> \"%4\"")
            .arg(javaPath, jarPath, outLog, errLog);
        c.arguments = { "/C", inner };
    } else {
        c.program = "/bin/sh";
        const QString inner = QString("exec '%1' -jar '%2' > '%3' 2> '%4'")
            .arg(javaPath, jarPath, outLog, errLog);
        c.arguments = { "-c", inner };
    }
    return c;
}

bool Launcher::run() {
    const LaunchCommand c = buildCommand(Paths::jrePath(), Paths::launcherFile(),
                                         Paths::launcherOutLog(), Paths::launcherErrLog(),
                                         currentOs());
    return QProcess::startDetached(c.program, c.arguments, Paths::baseDirectory());
}

}
```

- [ ] **Step 5: Build & run, verify PASS**

```bash
cmake --build build && ctest --test-dir build -R LauncherTest --output-on-failure
```

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/run/Launcher.* tests/LauncherTest.cpp
git commit -m "feat: detached launcher with log redirection (Launcher)"
```

---

## Task 11: app/Bootstrapper

**Files:**
- Create: `src/app/Bootstrapper.h`, `src/app/Bootstrapper.cpp`, `tests/BootstrapperTest.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `ProgressMonitor` (Task 7), `Downloader`/`JavaDownloader`/`LauncherDownloader` (Tasks 7–9), `Paths` (Task 4).
- Produces: free `tryExponential` and `Bootstrapper` QObject (used by main). The `tryExponential` retry behavior is the unit under test.

- [ ] **Step 1: Write the failing test** (deterministic retry, no real sleeps)

```cpp
#include <QtTest>
#include "app/Bootstrapper.h"
#include "app/ProgressMonitor.h"

using namespace tprunner;

namespace {
class NoopMonitor : public ProgressMonitor {
public:
    int statusCalls = 0;
    void setProgress(int) override {}
    void setMax(int) override {}
    void incrementProgress(int) override {}
    void setStatus(const QString&) override { ++statusCalls; }
};
}

class BootstrapperTest : public QObject {
    Q_OBJECT
private slots:
    void succeedsFirstTryNoSleep() {
        NoopMonitor mon; int sleeps = 0; int runs = 0;
        const bool ok = tryExponential(10, mon, [&]{ ++runs; },
                                       [&](int){ ++sleeps; });
        QVERIFY(ok); QCOMPARE(runs, 1); QCOMPARE(sleeps, 0);
    }
    void retriesThenSucceeds() {
        NoopMonitor mon; int sleeps = 0; int runs = 0;
        const bool ok = tryExponential(10, mon, [&]{
            if (++runs < 3) throw std::runtime_error("boom");
        }, [&](int){ ++sleeps; });
        QVERIFY(ok); QCOMPARE(runs, 3);
        // backoff seconds after attempt 0 (=1) and attempt 1 (=2) → 1 + 2 = 3 one-second sleeps
        QCOMPARE(sleeps, 3);
    }
    void returnsFalseAfterAllAttempts() {
        NoopMonitor mon; int runs = 0;
        const bool ok = tryExponential(3, mon, [&]{ ++runs; throw std::runtime_error("x"); },
                                       [](int){});
        QVERIFY(!ok); QCOMPARE(runs, 3);
    }
};

QTEST_APPLESS_MAIN(BootstrapperTest)
#include "BootstrapperTest.moc"
```

- [ ] **Step 2: Add to CMake and verify FAIL**

```cmake
target_sources(tprunner_lib PRIVATE src/app/Bootstrapper.cpp)
```
```cmake
tprunner_add_test(BootstrapperTest tests/BootstrapperTest.cpp)
```

- [ ] **Step 3: Write `src/app/Bootstrapper.h`**

```cpp
#pragma once
#include <QObject>
#include <QString>
#include <functional>
#include "app/ProgressMonitor.h"

namespace tprunner {

// Retry `block` up to `attemptsNumber` times. On failure of attempt N (0-based), wait
// 2^N seconds, showing a per-second countdown via `monitor.setStatus`, sleeping through
// `sleepSeconds(1)` each tick. Returns true on first success, false if all attempts fail.
bool tryExponential(int attemptsNumber, ProgressMonitor& monitor,
                    const std::function<void()>& block,
                    const std::function<void(int seconds)>& sleepSeconds);

class Bootstrapper : public QObject, public ProgressMonitor {
    Q_OBJECT
public:
    explicit Bootstrapper(QObject* parent = nullptr);

    void setProgress(int progress) override;
    void setMax(int max) override;
    void incrementProgress(int amount) override;
    void setStatus(const QString& status) override;

public slots:
    void run();

signals:
    void progressChanged(int progress);
    void maxChanged(int max);
    void statusChanged(const QString& status);
    void finished();

private:
    void checkAndDownloadAll();
    int current_ = 0;
};

}
```

- [ ] **Step 4: Write `src/app/Bootstrapper.cpp`**

```cpp
#include "app/Bootstrapper.h"
#include "net/Downloader.h"
#include "net/JavaDownloader.h"
#include "net/LauncherDownloader.h"
#include "util/Paths.h"
#include <QNetworkAccessManager>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QDebug>
#include <cmath>

namespace tprunner {

bool tryExponential(int attemptsNumber, ProgressMonitor& monitor,
                    const std::function<void()>& block,
                    const std::function<void(int seconds)>& sleepSeconds) {
    for (int attempt = 0; attempt < attemptsNumber; ++attempt) {
        try { block(); return true; }
        catch (const std::exception& ex) { qWarning() << "task failed:" << ex.what(); }
        catch (...) { qWarning() << "task failed (unknown)"; }

        const int waitSec = static_cast<int>(std::pow(2.0, attempt));
        for (int s = waitSec; s > 0; --s) {
            monitor.setStatus(QStringLiteral("Ошибка при загрузке. Попытка %1/%2 (%3с)")
                                  .arg(attempt).arg(attemptsNumber).arg(s));
            sleepSeconds(1);
        }
    }
    return false;
}

Bootstrapper::Bootstrapper(QObject* parent) : QObject(parent) {}

void Bootstrapper::setProgress(int p)        { current_ = p; emit progressChanged(p); }
void Bootstrapper::setMax(int m)             { emit maxChanged(m); }
void Bootstrapper::incrementProgress(int a)  { emit progressChanged(current_ + a); }
void Bootstrapper::setStatus(const QString& s){ emit statusChanged(s); }

void Bootstrapper::checkAndDownloadAll() {
    QNetworkAccessManager nam;
    Downloader dl(&nam);

    const QString jrePathFile = Paths::jrePathFile();
    QFile jf(jrePathFile);
    QString existing;
    if (jf.exists() && jf.open(QIODevice::ReadOnly)) { existing = QString::fromUtf8(jf.readAll()); jf.close(); }
    const bool needJre = existing.isEmpty() || !QFileInfo::exists(existing);
    if (needJre) {
        JavaDownloader jd(&dl);
        jd.init();
        const QString javaPath = jd.download(this);
        if (!javaPath.isEmpty()) Paths::writeJrePath(javaPath);
    }

    LauncherDownloader ld(&dl);
    ld.init();
    if (!ld.checkFile()) ld.update(this);
}

void Bootstrapper::run() {
    const bool ok = tryExponential(10, *this,
        [this]{ checkAndDownloadAll(); },
        [](int s){ QThread::sleep(s); });
    if (!ok)
        setStatus(QStringLiteral("Ошибка при загрузке. Проверьте подключение интернета"));
    emit finished();
}

}
```

- [ ] **Step 5: Build & run, verify PASS**

```bash
cmake --build build && ctest --test-dir build -R BootstrapperTest --output-on-failure
```

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/app/Bootstrapper.* tests/BootstrapperTest.cpp
git commit -m "feat: bootstrap orchestration + exponential retry (Bootstrapper)"
```

---

## Task 12: ui/GProgressBar

**Files:**
- Create: `src/ui/GProgressBar.h`, `src/ui/GProgressBar.cpp`, `tests/GProgressBarTest.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `GProgressBar` (used by SplashScreen).

- [ ] **Step 1: Write the failing test** (smoke: construct, set value, fixed height)

```cpp
#include <QtTest>
#include "ui/GProgressBar.h"

using namespace tprunner;

class GProgressBarTest : public QObject {
    Q_OBJECT
private slots:
    void hasFixedHeightAndAcceptsValue() {
        GProgressBar bar;
        bar.setRange(0, 100);
        bar.setValue(42);
        QCOMPARE(bar.value(), 42);
        QCOMPARE(bar.minimumHeight(), 20);
    }
};

QTEST_MAIN(GProgressBarTest)
#include "GProgressBarTest.moc"
```

- [ ] **Step 2: Add to CMake and verify FAIL**

```cmake
target_sources(tprunner_lib PRIVATE src/ui/GProgressBar.cpp)
```
```cmake
tprunner_add_test(GProgressBarTest tests/GProgressBarTest.cpp)
```

- [ ] **Step 3: Write `src/ui/GProgressBar.h`**

```cpp
#pragma once
#include <QProgressBar>

namespace tprunner {

class GProgressBar : public QProgressBar {
    Q_OBJECT
public:
    explicit GProgressBar(QWidget* parent = nullptr);
};

}
```

- [ ] **Step 4: Write `src/ui/GProgressBar.cpp`**

```cpp
#include "ui/GProgressBar.h"

namespace tprunner {

GProgressBar::GProgressBar(QWidget* parent) : QProgressBar(parent) {
    setTextVisible(false);
    setMinimumHeight(20);
    setStyleSheet(
        "QProgressBar {"
        "  border: none;"
        "  border-radius: 4px;"
        "  background: #ddddde;"
        "}"
        "QProgressBar::chunk {"
        "  border-radius: 4px;"
        "  background: #00db9d;"
        "}");
}

}
```

- [ ] **Step 5: Build & run, verify PASS**

```bash
cmake --build build && ctest --test-dir build -R GProgressBarTest --output-on-failure
```
Note: if the CI/runner is headless, run GUI tests with `QT_QPA_PLATFORM=offscreen ctest ...`.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/ui/GProgressBar.* tests/GProgressBarTest.cpp
git commit -m "feat: rounded green progress bar (GProgressBar)"
```

---

## Task 13: ui/SplashScreen + resources

**Files:**
- Create: `src/ui/SplashScreen.h`, `src/ui/SplashScreen.cpp`, `resources/resources.qrc`, `tests/SplashScreenTest.cpp`
- Move: `src/main/resources/background.jpeg` → `resources/background.jpeg`; `src/main/resources/close-btn.png` → `resources/close-btn.png`
- Modify: `CMakeLists.txt` (link qrc into `tprunner_lib`)

**Interfaces:**
- Consumes: `GProgressBar` (Task 12), `ProgressMonitor` slots semantics.
- Produces: `SplashScreen` widget with slots `onStatus/onProgress/onMax` and accessors `statusText()/progressValue()` (used by main).

- [ ] **Step 1: Move the image assets**

```bash
mkdir -p resources
git mv src/main/resources/background.jpeg resources/background.jpeg
git mv src/main/resources/close-btn.png   resources/close-btn.png
```

- [ ] **Step 2: Write `resources/resources.qrc`**

```xml
<RCC>
  <qresource prefix="/">
    <file>background.jpeg</file>
    <file>close-btn.png</file>
  </qresource>
</RCC>
```

- [ ] **Step 3: Write the failing test** (construct offscreen; slots update widgets)

```cpp
#include <QtTest>
#include "ui/SplashScreen.h"

using namespace tprunner;

class SplashScreenTest : public QObject {
    Q_OBJECT
private slots:
    void statusSlotUpdatesLabel() {
        SplashScreen s;
        s.onStatus("Проверка");
        QCOMPARE(s.statusText(), QStringLiteral("Проверка"));
    }
    void progressSlotUpdatesBar() {
        SplashScreen s;
        s.onMax(200);
        s.onProgress(50);
        QCOMPARE(s.progressValue(), 50);
    }
};

QTEST_MAIN(SplashScreenTest)
#include "SplashScreenTest.moc"
```

- [ ] **Step 4: Add to CMake and verify FAIL**

In `CMakeLists.txt`, add the qrc to the library (so tests linking the lib get the resources) and the source:
```cmake
target_sources(tprunner_lib PRIVATE src/ui/SplashScreen.cpp resources/resources.qrc)
```
```cmake
tprunner_add_test(SplashScreenTest tests/SplashScreenTest.cpp)
```
Run: build → compile error.

- [ ] **Step 5: Write `src/ui/SplashScreen.h`**

```cpp
#pragma once
#include <QWidget>

class QLabel;

namespace tprunner {

class GProgressBar;

class SplashScreen : public QWidget {
    Q_OBJECT
public:
    explicit SplashScreen(QWidget* parent = nullptr);

    QString statusText() const;
    int progressValue() const;

public slots:
    void onStatus(const QString& status);
    void onProgress(int progress);
    void onMax(int max);

private:
    QLabel* background_ = nullptr;
    QLabel* label_ = nullptr;
    GProgressBar* bar_ = nullptr;
};

}
```

- [ ] **Step 6: Write `src/ui/SplashScreen.cpp`**

```cpp
#include "ui/SplashScreen.h"
#include "ui/GProgressBar.h"
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>

namespace tprunner {

namespace {
QString pickFont() {
    const QStringList families = QFontDatabase::families();
    for (const QString& wanted : {QStringLiteral("Helvetica"), QStringLiteral("Arial")})
        if (families.contains(wanted)) return wanted;
    return QString();
}
}

SplashScreen::SplashScreen(QWidget* parent) : QWidget(parent) {
    setWindowFlag(Qt::FramelessWindowHint);
    setWindowTitle(QStringLiteral("Загрузка..."));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Background with overlaid close button.
    background_ = new QLabel(this);
    const QPixmap bg(QStringLiteral(":/background.jpeg"));
    background_->setPixmap(bg);
    background_->setFixedSize(bg.size());

    auto* closeBtn = new QPushButton(background_);
    const QPixmap closeIcon(QStringLiteral(":/close-btn.png"));
    closeBtn->setIcon(QIcon(closeIcon));
    closeBtn->setIconSize(closeIcon.size());
    closeBtn->setFixedSize(closeIcon.size());
    closeBtn->setFlat(true);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("border: none; background: transparent;");
    closeBtn->move(bg.width() - closeIcon.width() - 13, 13);
    connect(closeBtn, &QPushButton::clicked, this, []{ std::exit(0); });

    root->addWidget(background_);

    // Status panel.
    auto* statusPanel = new QWidget(this);
    statusPanel->setStyleSheet("background: #303135;");
    auto* statusLayout = new QVBoxLayout(statusPanel);
    statusLayout->setContentsMargins(20, 15, 20, 15);
    statusLayout->setSpacing(8);

    label_ = new QLabel(QStringLiteral("Загрузка..."), statusPanel);
    label_->setAlignment(Qt::AlignCenter);
    QFont font(pickFont()); font.setPointSize(14); label_->setFont(font);
    label_->setStyleSheet("color: #ddddde;");
    statusLayout->addWidget(label_);

    bar_ = new GProgressBar(statusPanel);
    statusLayout->addWidget(bar_);

    root->addWidget(statusPanel);

    setFixedSize(sizeHint());
    if (auto* scr = QGuiApplication::primaryScreen())
        move(scr->geometry().center() - rect().center());
}

QString SplashScreen::statusText() const { return label_->text(); }
int SplashScreen::progressValue() const { return bar_->value(); }

void SplashScreen::onStatus(const QString& status) { label_->setText(status); }
void SplashScreen::onProgress(int progress) { bar_->setValue(progress); }
void SplashScreen::onMax(int max) { bar_->setMaximum(max); }

}
```

- [ ] **Step 7: Build & run, verify PASS**

```bash
cmake -B build -S . && cmake --build build && QT_QPA_PLATFORM=offscreen ctest --test-dir build -R SplashScreenTest --output-on-failure
```

- [ ] **Step 8: Commit**

```bash
git add CMakeLists.txt src/ui/SplashScreen.* resources/ tests/SplashScreenTest.cpp
git commit -m "feat: frameless splash screen + embedded resources (SplashScreen)"
```

---

## Task 14: main.cpp — wiring

**Files:**
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: `SplashScreen` (Task 13), `Bootstrapper` (Task 11), `Launcher` (Task 10).
- Produces: the running application.

This task has no unit test (it launches a real process + network). Verify manually per the checklist in Step 3.

- [ ] **Step 1: Replace `src/main.cpp`**

```cpp
#include <QApplication>
#include <QThread>
#include "ui/SplashScreen.h"
#include "app/Bootstrapper.h"
#include "run/Launcher.h"

using namespace tprunner;

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    SplashScreen splash;
    splash.show();

    auto* thread = new QThread(&app);
    auto* boot = new Bootstrapper;
    boot->moveToThread(thread);

    QObject::connect(boot, &Bootstrapper::statusChanged,   &splash, &SplashScreen::onStatus);
    QObject::connect(boot, &Bootstrapper::progressChanged, &splash, &SplashScreen::onProgress);
    QObject::connect(boot, &Bootstrapper::maxChanged,      &splash, &SplashScreen::onMax);
    QObject::connect(thread, &QThread::started, boot, &Bootstrapper::run);

    QObject::connect(boot, &Bootstrapper::finished, &app, [&] {
        splash.hide();
        Launcher::run();
        thread->quit();
        thread->wait();
        app.exit(0);
    });

    thread->start();
    return app.exec();
}
```

- [ ] **Step 2: Build**

```bash
cmake --build build
```
Expected: `TechnoRunner` builds with no errors.

- [ ] **Step 3: Manual verification checklist**

Run `./build/TechnoRunner` (on a machine with network) and confirm:
- Splash appears, frameless, centered, with background + close button (top-right).
- Status text transitions through "Загрузка Java..." / "Скачивание лаунчера..." as needed; progress bar fills green.
- `~/.minecraft/technomine/` (Linux) gets `jre/`, `jrepath.txt`, `launcher.jar`.
- The launcher process starts; `launcherout.log`/`launchererr.log` appear; the runner window closes and the process exits.
- Re-running with everything present skips downloads and launches immediately.
- Clicking the close button exits.

- [ ] **Step 4: Commit**

```bash
git add src/main.cpp
git commit -m "feat: wire splash + worker bootstrap + launch (main)"
```

---

## Task 15: Migration cleanup

**Files:**
- Preserve then delete: copy `src/pkg/macos/app/Info.plist` + `Resources/icon.icns` into `resources/macos/`
- Delete: `src/main/` (Kotlin/Java), `src/pkg/`, `build.gradle`, `settings.gradle`, `gradle/`, `gradlew`, `gradlew.bat`, `linux.Makefile`, `macos.Makefile`, `windows.Makefile`, `.travis.yml`, `scripts/macos_pkg_dmg.sh`
- Keep: `scripts/Background.png`, `jres.json`, `launcher.json`
- Modify: `README.md`

**Interfaces:**
- Produces: a clean repo containing only the Qt project. Build must still pass.

- [ ] **Step 1: Preserve macOS bundle assets, then delete the old project**

```bash
mkdir -p resources/macos
git mv src/pkg/macos/app/Info.plist resources/macos/Info.plist.in
git mv src/pkg/macos/app/Resources/icon.icns resources/macos/icon.icns
git rm -r src/main src/pkg gradle
git rm build.gradle settings.gradle gradlew gradlew.bat
git rm linux.Makefile macos.Makefile windows.Makefile .travis.yml scripts/macos_pkg_dmg.sh
```

- [ ] **Step 2: Make `Info.plist.in` a CMake-substitutable template**

Edit `resources/macos/Info.plist.in` so `CFBundleExecutable` uses the CMake var (keep all other keys, incl. `LSUIElement`/`NSHighResolutionCapable`/`CFBundleIdentifier=ru.glitchless.games`):

```xml
    <key>CFBundleExecutable</key>
    <string>${MACOSX_BUNDLE_EXECUTABLE_NAME}</string>
    <key>CFBundleIconFile</key>
    <string>icon.icns</string>
```

- [ ] **Step 3: Wire macOS bundle props into CMake**

In `CMakeLists.txt`, after `qt_add_executable(TechnoRunner ...)` add:

```cmake
if(APPLE)
    set_target_properties(TechnoRunner PROPERTIES
        OUTPUT_NAME "glitchless-minecraft"
        MACOSX_BUNDLE_BUNDLE_NAME "Minecraft"
        MACOSX_BUNDLE_GUI_IDENTIFIER "ru.glitchless.games"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/macos/Info.plist.in")
    target_sources(TechnoRunner PRIVATE resources/macos/icon.icns)
    set_source_files_properties(resources/macos/icon.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
endif()
```

- [ ] **Step 4: Rewrite `README.md`**

```markdown
# TechnoRunner

Native (C++/Qt 6) bootstrapper for the Technopark / glitchless Minecraft launcher.
On first run it downloads an OS/arch-matched JRE and the launcher JAR into
`<app-data>/.minecraft/technomine`, verifies the JAR's SHA-256, then launches it.

## Build

Requires Qt 6 (Widgets, Network, Test), CMake ≥ 3.21, and libarchive.

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build
ctest --test-dir build --output-on-failure
```

Run: `./build/TechnoRunner`
```

- [ ] **Step 5: Verify the build & tests still pass**

```bash
cmake -B build -S . && cmake --build build && QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```
Expected: all tests PASS; no references to deleted files remain (`grep -rn "src/main\|src/pkg" CMakeLists.txt` → empty).

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "chore: remove Kotlin/Swing runner and native pkg bootstrap; wire macOS bundle"
```

---

## Task 16: GitHub Actions CI

**Files:**
- Create: `.github/workflows/build.yml`

**Interfaces:**
- Produces: cross-platform build + test + release artifacts (AppImage / zip / dmg). Replaces Travis.

- [ ] **Step 1: Write `.github/workflows/build.yml`**

```yaml
name: build
on:
  push:
    branches: [ master, qt-rewrite ]
    tags: [ 'v*' ]
  pull_request:

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - { os: ubuntu-latest,  name: linux-x86_64,  arch: x86_64 }
          - { os: windows-latest, name: windows-x86_64, arch: x86_64 }
          - { os: macos-13,       name: macos-x86_64,   arch: x86_64 }
          - { os: macos-14,       name: macos-arm64,    arch: arm64 }
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4

      - name: Install Qt
        uses: jurplel/install-qt-action@v4
        with:
          version: '6.8.*'
          cache: true

      - name: Install libarchive (Linux)
        if: runner.os == 'Linux'
        run: sudo apt-get update && sudo apt-get install -y libarchive-dev libfuse2

      - name: Install libarchive (macOS)
        if: runner.os == 'macOS'
        run: brew install libarchive

      - name: Install libarchive (Windows)
        if: runner.os == 'Windows'
        run: vcpkg install libarchive:x64-windows

      - name: Configure
        shell: bash
        run: |
          if [ "${{ runner.os }}" = "Windows" ]; then
            cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake"
          elif [ "${{ runner.os }}" = "macOS" ]; then
            cmake -B build -S . -DCMAKE_PREFIX_PATH="$(brew --prefix libarchive)"
          else
            cmake -B build -S .
          fi

      - name: Build
        run: cmake --build build --config Release

      - name: Test
        run: ctest --test-dir build --output-on-failure
        env:
          QT_QPA_PLATFORM: offscreen

      - name: Package (Linux AppImage)
        if: runner.os == 'Linux'
        run: |
          curl -fsSLO https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
          curl -fsSLO https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
          chmod +x linuxdeploy*.AppImage
          mkdir -p AppDir/usr/bin && cp build/TechnoRunner AppDir/usr/bin/
          # minimal .desktop + icon
          mkdir -p AppDir/usr/share/applications AppDir/usr/share/icons/hicolor/256x256/apps
          printf '[Desktop Entry]\nType=Application\nName=TechnoRunner\nExec=TechnoRunner\nIcon=technorunner\nCategories=Game;\n' > AppDir/usr/share/applications/technorunner.desktop
          cp resources/background.jpeg AppDir/usr/share/icons/hicolor/256x256/apps/technorunner.png || true
          ./linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt --output appimage
          mv TechnoRunner*.AppImage TechnoRunner-${{ matrix.name }}.AppImage

      - name: Package (macOS dmg)
        if: runner.os == 'macOS'
        run: |
          macdeployqt build/glitchless-minecraft.app -dmg
          mv build/glitchless-minecraft.dmg TechnoRunner-${{ matrix.name }}.dmg

      - name: Package (Windows zip)
        if: runner.os == 'Windows'
        shell: bash
        run: |
          mkdir dist && cp build/Release/TechnoRunner.exe dist/ 2>/dev/null || cp build/TechnoRunner.exe dist/
          windeployqt dist/TechnoRunner.exe
          7z a TechnoRunner-${{ matrix.name }}.zip ./dist/*

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: TechnoRunner-${{ matrix.name }}
          path: |
            TechnoRunner-*.AppImage
            TechnoRunner-*.dmg
            TechnoRunner-*.zip

      - name: Release on tag
        if: startsWith(github.ref, 'refs/tags/')
        uses: softprops/action-gh-release@v2
        with:
          files: |
            TechnoRunner-*.AppImage
            TechnoRunner-*.dmg
            TechnoRunner-*.zip
```

- [ ] **Step 2: Lint the YAML locally**

```bash
python3 -c "import yaml,sys; yaml.safe_load(open('.github/workflows/build.yml')); print('yaml ok')"
```
Expected: `yaml ok`.

- [ ] **Step 3: Commit**

```bash
git add .github/workflows/build.yml
git commit -m "ci: cross-platform GitHub Actions build/test/release (replaces Travis)"
```

---

## Task 17 (optional): technorunner-hasher dev utility

**Files:**
- Create: `src/tools/hasher_main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `sha256Base64` (Task 2). Mirrors the old `HasherMain.kt` — prints a file's Base64 SHA-256 (for filling `launcher.json`).

- [ ] **Step 1: Write `src/tools/hasher_main.cpp`**

```cpp
#include "util/Hash.h"
#include <QCoreApplication>
#include <cstdio>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { std::fprintf(stderr, "usage: technorunner-hasher <file>\n"); return 2; }
    std::printf("%s\n", tprunner::sha256Base64(QString::fromLocal8Bit(argv[1])).toUtf8().constData());
    return 0;
}
```

- [ ] **Step 2: Add an option-gated target to CMake**

```cmake
option(TPRUNNER_BUILD_HASHER "Build the SHA-256 hasher utility" OFF)
if(TPRUNNER_BUILD_HASHER)
    qt_add_executable(technorunner-hasher src/tools/hasher_main.cpp)
    target_link_libraries(technorunner-hasher PRIVATE tprunner_lib)
endif()
```

- [ ] **Step 3: Build with the option and smoke-test**

```bash
cmake -B build -S . -DTPRUNNER_BUILD_HASHER=ON && cmake --build build --target technorunner-hasher
./build/technorunner-hasher CMakeLists.txt
```
Expected: prints a Base64 string.

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt src/tools/hasher_main.cpp
git commit -m "feat: optional SHA-256 hasher utility (HasherMain port)"
```

---

## Self-Review (completed by plan author)

**Spec coverage:**
- §2 external contract → Tasks 2 (hash), 3 (os/arch), 4 (paths/URLs via Paths + endpoints in 8/9), 5 (models), 11 (retry/strings), 10 (launch). ✓
- §3 architecture (worker thread + signals) → Tasks 11, 13, 14. ✓
- §4 components → Tasks 2–14 (one each). ✓
- §5 UI fidelity → Tasks 12, 13. ✓
- §6 error handling → Task 11 (tryExponential), 9 (sha mismatch keeps jar), 8 (null match). ✓
- §7 build/deps → Tasks 1, 16. ✓
- §8 improvements: single binary (1/15), QProcess detached (10), runner.log (Paths Task 4 exposes it; wired via qInstall option — see note), HiDPI (Qt6 default), streaming download (7). ✓
- §9 CI/packaging + macOS plist reuse → Tasks 15, 16. ✓
- §10 testing → every component task has Qt Test. ✓
- §11 migration → Task 15. ✓

**Note on runner.log (improvement #3):** `Paths::runnerLog()` is provided (Task 4). If desired, install a `qInstallMessageHandler` in `main.cpp` (Task 14) writing to `Paths::runnerLog()`; this is a 6-line addition and can be folded into Task 14 Step 1 if the executor wants persistent diagnostics. Left optional to avoid over-scoping.

**Placeholder scan:** No TBD/TODO; all code blocks complete. The one intentional placeholder is the deliberately-failing `QCOMPARE(2+2,5)` in Task 1 (TDD red step), fixed in the same task.

**Type consistency:** `ProgressMonitor`, `Downloader`, `Os`/`CpuArch`, `Paths::*`, `JavaBinaryModel`/`LauncherModel`, `LaunchCommand` names/signatures match across the contract block and all consuming tasks. `findMatch`, `buildCommand`, `tryExponential` signatures consistent between definition and tests.
