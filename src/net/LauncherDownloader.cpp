#include "net/LauncherDownloader.h"
#include "net/Downloader.h"
#include "app/ProgressMonitor.h"
#include "util/Paths.h"
#include "util/Hash.h"
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QDebug>
#include <exception>

namespace tprunner {

namespace { const char* kLauncherUrl = "https://github.com/glitchless/TechnoparkLauncher/releases/latest/download/launcher.json"; }

LauncherDownloader::LauncherDownloader(Downloader* dl) : dl_(dl) {}

QByteArray LauncherDownloader::fetchManifest() { return dl_->httpGet(kLauncherUrl); }

void LauncherDownloader::init() {
    qInfo().noquote() << "find: fetching launcher manifest" << kLauncherUrl;
    try { model_ = LauncherModel::fromJson(fetchManifest()); }
    catch (const std::exception& ex) {
        qWarning().noquote() << "find: manifest fetch FAILED:" << ex.what();
        model_ = std::nullopt;
    }
    catch (...) {
        qWarning().noquote() << "find: manifest fetch FAILED (unknown)";
        model_ = std::nullopt;
    }
    // fromJson returns nullopt (no throw) on invalid/non-object JSON, so guard the
    // dereference — accessing an empty optional here is UB (was a SIGBUS crash).
    if (model_)
        qInfo().noquote() << "find: manifest parsed — jreCode" << model_->jreCode
                          << "jreFiles" << model_->jreFiles.size()
                          << "launcherFiles" << model_->files.size();
    else
        qWarning() << "find: manifest empty or invalid → no model";
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
    if (!QFileInfo::exists(Paths::launcherFile())) {
        qInfo().noquote() << "find: launcher.jar absent at" << Paths::launcherFile();
        return false;
    }
    if (!model_) {
        qWarning() << "verify: launcher.jar present but no manifest to verify against → assuming OK";
        return true;                                // can't verify → assume OK (matches original)
    }
    const bool ok = sha256Base64(Paths::launcherFile()) == selectedJarSha();
    qInfo().noquote() << "verify: existing launcher.jar hash match =" << ok;
    return ok;
}

void LauncherDownloader::update(ProgressMonitor* monitor) {
    if (!model_) { qWarning() << "download: launcher update skipped — no manifest"; return; }
    const QString url = selectedJarUrl();
    if (url.isEmpty()) { qWarning() << "find: launcher update skipped — no jar url for platform"; return; }
    qInfo().noquote() << "download: launcher jar" << url;
    if (monitor) monitor->setStatus(QObject::tr("Скачивание лаунчера..."));
    const QString updateFile = Paths::temporaryDirectory() + "/update_launcher.jar";
    dl_->downloadToFile(url, updateFile, monitor);
    if (monitor) monitor->setProgress(100);

    const QString sha = selectedJarSha();
    if (!sha.isEmpty() && sha256Base64(updateFile) != sha) {
        qWarning().noquote() << "verify: downloaded launcher.jar hash MISMATCH → keeping old jar";
        return;   // verification failed → keep old jar
    }
    qInfo() << "verify: downloaded launcher.jar hash ok";

    QFile launcher(Paths::launcherFile());
    if (!launcher.exists() || launcher.remove()) {
        QFile::rename(updateFile, Paths::launcherFile());
        qInfo().noquote() << "find: installed new launcher.jar at" << Paths::launcherFile();
    } else {
        qWarning().noquote() << "find: could not replace old launcher.jar at" << Paths::launcherFile();
    }
}

}
