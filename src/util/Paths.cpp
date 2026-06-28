#include "util/Paths.h"
#include "util/Platform.h"
#include <QDir>
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
QString Paths::manifestFile()       { return baseDirectory() + "/launcher.json"; }
QString Paths::launcherFile()       { return baseDirectory() + "/launcher.jar"; }
QString Paths::launcherOutLog()     { return baseDirectory() + "/launcherout.log"; }
QString Paths::launcherErrLog()     { return baseDirectory() + "/launchererr.log"; }
QString Paths::runnerLog()          { return baseDirectory() + "/runner.log"; }

}
