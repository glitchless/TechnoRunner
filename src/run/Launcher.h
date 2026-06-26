#pragma once
#include <QString>
#include <QStringList>

namespace tprunner {

// A resolved launch: the program to run, its arguments, and the files stdout/stderr
// are redirected to. Redirection is data (handled by QProcess's own channels), not a
// shell `>` baked into the arguments — see Launcher::buildCommand.
struct LaunchCommand {
    QString program;
    QStringList arguments;
    QString outLog;
    QString errLog;
};

class Launcher {
public:
    static LaunchCommand buildCommand(const QString& javaPath, const QString& jarPath,
                                      const QString& outLog, const QString& errLog);
    // Launch `jarPath` with `javaPath`. If javaPath is empty or missing, falls back to
    // "java" on PATH. Returns QProcess::startDetached's result.
    static bool run(const QString& javaPath, const QString& jarPath);
};

}
