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

QString LauncherDownloader::selectedJarUrl() const {
    if (!model_) return QString();
    if (const auto m = selectForCurrentPlatform(model_->files)) return m->downloadUrl;
    return model_->downloadUrl;
}

QString LauncherDownloader::selectedJarSha() const {
    if (!model_) return QString();
    if (const auto m = selectForCurrentPlatform(model_->files)) return m->sha256;
    return model_->sha256;
}

bool LauncherDownloader::checkFile() {
    if (!QFileInfo::exists(Paths::launcherFile())) return false;
    if (!model_) return true;                       // can't verify → assume OK (matches original)
    return sha256Base64(Paths::launcherFile()) == selectedJarSha();
}

void LauncherDownloader::update(ProgressMonitor* monitor) {
    if (!model_) return;
    const QString url = selectedJarUrl();
    if (url.isEmpty()) return;
    if (monitor) monitor->setStatus(QObject::tr("Скачивание лаунчера..."));
    const QString updateFile = Paths::temporaryDirectory() + "/update_launcher.jar";
    dl_->downloadToFile(url, updateFile, monitor);
    if (monitor) monitor->setProgress(100);

    const QString sha = selectedJarSha();
    if (!sha.isEmpty() && sha256Base64(updateFile) != sha) return;   // verification failed → keep old jar

    QFile launcher(Paths::launcherFile());
    if (!launcher.exists() || launcher.remove())
        QFile::rename(updateFile, Paths::launcherFile());
}

}
