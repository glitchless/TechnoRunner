#include "models/Models.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

namespace tprunner {

JavaBinaryModel JavaBinaryModel::fromJsonObject(const QJsonObject& o) {
    JavaBinaryModel m;
    m.type             = o.value("type").toString();
    m.arch             = o.value("arch").toString();
    m.downloadUrl      = o.value("downloadUrl").toString();
    m.javaRelativePath = o.value("javaRelativePath").toString();
    m.extension        = o.value("extension").toString();
    return m;
}

QList<JavaBinaryModel> JavaBinaryModel::listFromJson(const QByteArray& json) {
    QList<JavaBinaryModel> out;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) return out;
    for (const QJsonValue& v : doc.array())
        out.append(fromJsonObject(v.toObject()));
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

    const QJsonObject jre = o.value("jre").toObject();
    m.jreCode = jre.value("code").toString();
    for (const QJsonValue& v : jre.value("files").toArray())
        m.jreFiles.append(JavaBinaryModel::fromJsonObject(v.toObject()));

    return m;
}

}
