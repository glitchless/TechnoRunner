#pragma once
#include <QString>
#include <QList>
#include <QByteArray>
#include <optional>

class QJsonObject;

namespace tprunner {

struct JavaBinaryModel {
    QString type, arch, downloadUrl, javaRelativePath, extension;
    QString sha256;   // Base64 SHA-256 of the archive ("SHA-256" key); empty if absent
    static JavaBinaryModel fromJsonObject(const QJsonObject& o);
    static QList<JavaBinaryModel> listFromJson(const QByteArray& json);
};

struct LauncherModel {
    QString version, downloadUrl, sha256;
    // Embedded JRE descriptor (new launcher.json format): `jre.code` names the
    // install subfolder, `jre.files` are the per-OS/arch JRE binaries.
    QString jreCode;
    QList<JavaBinaryModel> jreFiles;
    static std::optional<LauncherModel> fromJson(const QByteArray& json);
};

}
