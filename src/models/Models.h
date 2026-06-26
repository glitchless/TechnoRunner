#pragma once
#include <QString>
#include <QList>
#include <QByteArray>
#include <optional>

class QJsonObject;

namespace tprunner {

enum class Os;
enum class CpuArch;

struct JavaBinaryModel {
    QString type, arch, downloadUrl, javaRelativePath, extension;
    QString sha256;       // Base64 SHA-256 of the archive ("SHA-256" key); empty if absent
    QString javaSha256;   // Base64 SHA-256 of the extracted java binary ("javaSHA-256"); empty if absent
    static JavaBinaryModel fromJsonObject(const QJsonObject& o);
    static QList<JavaBinaryModel> listFromJson(const QByteArray& json);
};

struct LauncherModel {
    QString version, downloadUrl, sha256;   // single top-level jar (fallback)
    // Per-OS/arch launcher jars ("files"); preferred over the single jar when an entry
    // matches the current platform. Reuses JavaBinaryModel (extension/javaRelativePath
    // unused for jars).
    QList<JavaBinaryModel> files;
    // Embedded JRE descriptor (new launcher.json format): `jre.code` names the
    // install subfolder, `jre.files` are the per-OS/arch JRE binaries.
    QString jreCode;
    QList<JavaBinaryModel> jreFiles;
    static std::optional<LauncherModel> fromJson(const QByteArray& json);
};

// First entry whose type/arch match the given / current machine, or nullopt.
std::optional<JavaBinaryModel> matchBinary(const QList<JavaBinaryModel>& list, Os os, CpuArch arch);
std::optional<JavaBinaryModel> selectForCurrentPlatform(const QList<JavaBinaryModel>& list);

}
