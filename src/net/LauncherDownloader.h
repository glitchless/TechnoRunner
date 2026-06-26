#pragma once
#include <QString>
#include <optional>
#include "models/Models.h"

namespace tprunner {

class Downloader;
class ProgressMonitor;

class LauncherDownloader {
public:
    explicit LauncherDownloader(Downloader* dl);
    virtual ~LauncherDownloader() = default;
    void init();
    bool checkFile();
    void update(ProgressMonitor* monitor);
    QString jreCode() const;                   // from manifest's jre.code ("" if none)
    QList<JavaBinaryModel> jreFiles() const;    // from manifest's jre.files
protected:
    virtual QByteArray fetchManifest();   // seam for tests
    Downloader* dl_;
    std::optional<LauncherModel> model_;
};

}
