#include "run/Launcher.h"
#include "util/Platform.h"
#include "util/Paths.h"
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

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
    const QString java = Paths::jrePath();
    const QString jar  = Paths::launcherFile();
    qInfo().noquote() << "launch: java" << java << "(exists" << QFileInfo::exists(java) << ")";
    qInfo().noquote() << "launch: jar" << jar << "(exists" << QFileInfo::exists(jar) << ")";

    const LaunchCommand c = buildCommand(java, jar,
                                         Paths::launcherOutLog(), Paths::launcherErrLog(),
                                         currentOs());
    qInfo().noquote() << "launch: exec" << c.program << c.arguments
                      << "cwd" << Paths::baseDirectory();
    const bool ok = QProcess::startDetached(c.program, c.arguments, Paths::baseDirectory());
    if (ok) qInfo()  << "launch: startDetached ok";
    else    qWarning() << "launch: startDetached FAILED";
    return ok;
}

}
