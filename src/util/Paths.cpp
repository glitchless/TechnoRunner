#include "util/Paths.h"
#include "util/Platform.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
QString Paths::javaDirectory(const QString& code) { const QString d = baseDirectory() + "/jre/" + code; QDir().mkpath(d); return d; }
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
