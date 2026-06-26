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
protected:
    virtual QByteArray fetchManifest();   // seam for tests
    Downloader* dl_;
    std::optional<LauncherModel> model_;
};

}
