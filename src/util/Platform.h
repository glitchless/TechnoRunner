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
