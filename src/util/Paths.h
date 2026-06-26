#pragma once
#include <QString>

namespace tprunner {

class Paths {
public:
    static QString workingDirectory();
    static QString baseDirectory();
    static QString temporaryDirectory();
    static QString javaDirectory();
    static QString jrePathFile();
    static void    writeJrePath(const QString& absoluteJavaPath);
    static QString jrePath();
    static QString launcherFile();
    static QString launcherOutLog();
    static QString launcherErrLog();
    static QString runnerLog();

    static void setBaseOverride(const QString& dir); // tests only; "" clears
};

}
