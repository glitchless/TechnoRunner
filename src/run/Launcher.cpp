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
