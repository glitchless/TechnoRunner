#include "run/Launcher.h"
#include "util/Paths.h"
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

namespace tprunner {

LaunchCommand Launcher::buildCommand(const QString& javaPath, const QString& jarPath,
                                     const QString& outLog, const QString& errLog) {
    // Launch java directly — no shell. Routing through `cmd.exe /C "<command>"` is
    // broken on Windows: QProcess re-quotes the inner argument and escapes its
    // embedded quotes as \" , which cmd.exe cannot parse ("The filename, directory
    // name, or volume label syntax is incorrect"), so java never starts. Output is
    // redirected via QProcess's stdout/stderr file channels (see run()) instead of a
    // shell `>`, which is portable and quote-safe on every platform.
    LaunchCommand c;
    c.program = javaPath;
    c.arguments = { QStringLiteral("-jar"), jarPath };
    c.outLog = outLog;
    c.errLog = errLog;
    return c;
}

bool Launcher::run(const QString& javaPath, const QString& jarPath) {
    // Fall back to "java" on PATH when the manifest resolved no usable JRE (e.g. offline
    // with no cached manifest) — best-effort launch, matching the old behaviour.
    QString java = javaPath;
    if (java.isEmpty() || !QFileInfo::exists(java)) {
        qWarning().noquote() << "launch: resolved java" << (java.isEmpty() ? QStringLiteral("(none)") : java)
                             << "unusable — falling back to PATH 'java'";
        java = QStringLiteral("java");
    }
    const QString jar = jarPath;
    qInfo().noquote() << "launch: java" << java;
    qInfo().noquote() << "launch: jar" << jar << "(exists" << QFileInfo::exists(jar) << ")";

    const LaunchCommand c = buildCommand(java, jar,
                                         Paths::launcherOutLog(), Paths::launcherErrLog());
    qInfo().noquote() << "launch: exec" << c.program << c.arguments
                      << "cwd" << Paths::baseDirectory();

    // The non-static startDetached honors the program/arguments/working-directory and
    // the stdout/stderr file channels set below; the static overload does not, and its
    // argument re-quoting is what broke the old cmd.exe path.
    QProcess p;
    p.setProgram(c.program);
    p.setArguments(c.arguments);
    p.setWorkingDirectory(Paths::baseDirectory());
    p.setStandardOutputFile(c.outLog);
    p.setStandardErrorFile(c.errLog);

    qint64 pid = 0;
    const bool ok = p.startDetached(&pid);
    if (ok) qInfo().noquote()  << "launch: startDetached ok pid" << pid;
    else    qWarning().noquote() << "launch: startDetached FAILED:" << p.errorString();
    return ok;
}

}
