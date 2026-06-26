#include "net/LauncherDownloader.h"
#include "net/Downloader.h"
#include "app/ProgressMonitor.h"
#include "util/Paths.h"
#include "util/Hash.h"
#include <QFile>
#include <QFileInfo>
#include <QObject>

namespace tprunner {

namespace { const char* kLauncherUrl = "https://minecraft.glitchless.ru/launcher.json"; }

LauncherDownloader::LauncherDownloader(Downloader* dl) : dl_(dl) {}

QByteArray LauncherDownloader::fetchManifest() { return dl_->httpGet(kLauncherUrl); }

void LauncherDownloader::init() {
    try { model_ = LauncherModel::fromJson(fetchManifest()); }
    catch (...) { model_ = std::nullopt; }
}

QString LauncherDownloader::jreCode() const { return model_ ? model_->jreCode : QString(); }
QList<JavaBinaryModel> LauncherDownloader::jreFiles() const { return model_ ? model_->jreFiles : QList<JavaBinaryModel>(); }

bool LauncherDownloader::checkFile() {
    if (!QFileInfo::exists(Paths::launcherFile())) return false;
    if (!model_) return true;                       // can't verify → assume OK (matches original)
    return sha256Base64(Paths::launcherFile()) == model_->sha256;
}

void LauncherDownloader::update(ProgressMonitor* monitor) {
    if (!model_) return;
    if (monitor) monitor->setStatus(QObject::tr("Скачивание лаунчера..."));
    const QString updateFile = Paths::temporaryDirectory() + "/update_launcher.jar";
    dl_->downloadToFile(model_->downloadUrl, updateFile, monitor);
    if (monitor) monitor->setProgress(100);

    if (sha256Base64(updateFile) != model_->sha256) return;   // verification failed → keep old jar

    QFile launcher(Paths::launcherFile());
    if (!launcher.exists() || launcher.remove())
        QFile::rename(updateFile, Paths::launcherFile());
}

}
