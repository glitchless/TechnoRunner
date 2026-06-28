#pragma once
#include <QString>

namespace tprunner {

class Archive {
public:
    static void extractZip(const QString& archivePath, const QString& destDir);
    static void extractTarGz(const QString& archivePath, const QString& destDir);
};

}
