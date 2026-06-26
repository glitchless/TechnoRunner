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
    // The jar to use for this machine: the per-arch "files" entry matching the current
    // platform, else the single top-level downloadFullPath/SHA-256.
    QString selectedJarUrl() const;
    QString selectedJarSha() const;
    Downloader* dl_;
    std::optional<LauncherModel> model_;
};

}
