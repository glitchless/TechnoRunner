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
