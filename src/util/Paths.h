#pragma once
#include <QString>

namespace tprunner {

class Paths {
public:
    static QString workingDirectory();
    static QString baseDirectory();
    static QString temporaryDirectory();
    static QString javaDirectory();
    static QString javaDirectory(const QString& code); // <base>/jre/<code>
    static QString manifestFile();                     // <base>/launcher.json (cached manifest)
    static QString launcherFile();
    static QString launcherOutLog();
    static QString launcherErrLog();
    static QString runnerLog();

    static void setBaseOverride(const QString& dir); // tests only; "" clears
};

}
