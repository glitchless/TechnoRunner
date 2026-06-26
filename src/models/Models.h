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
