#include "app/Bootstrapper.h"
#include "net/Downloader.h"
#include "net/JavaDownloader.h"
#include "net/LauncherDownloader.h"
#include "util/Paths.h"
#include <QNetworkAccessManager>
#include <QFileInfo>
#include <QThread>
#include <QDebug>
#include <cmath>

namespace tprunner {

bool tryExponential(int attemptsNumber, ProgressMonitor& monitor,
                    const std::function<void()>& block,
                    const std::function<void(int seconds)>& sleepSeconds) {
    for (int attempt = 0; attempt < attemptsNumber; ++attempt) {
        try { block(); return true; }
        catch (const std::exception& ex) { qWarning() << "task failed:" << ex.what(); }
        catch (...) { qWarning() << "task failed (unknown)"; }

        const int waitSec = static_cast<int>(std::pow(2.0, attempt));
        for (int s = waitSec; s > 0; --s) {
            monitor.setStatus(QStringLiteral("Ошибка при загрузке. Попытка %1/%2 (%3с)")
                                  .arg(attempt).arg(attemptsNumber).arg(s));
            sleepSeconds(1);
        }
    }
    return false;
}

Bootstrapper::Bootstrapper(QObject* parent) : QObject(parent) {}

void Bootstrapper::setProgress(int p)        { current_ = p; emit progressChanged(p); }
void Bootstrapper::setMax(int m)             { emit maxChanged(m); }
void Bootstrapper::incrementProgress(int a)  { emit progressChanged(current_ + a); }
void Bootstrapper::setStatus(const QString& s){ emit statusChanged(s); }

void Bootstrapper::checkAndDownloadAll() {
    QNetworkAccessManager nam;
    Downloader dl(&nam);

    // launcher.json is the single manifest: it carries both the launcher jar info
    // and the embedded JRE descriptor (jre.code + jre.files).
    LauncherDownloader ld(&dl);
    ld.init();

    // JRE: the java path is derived from the manifest (jre/<jreCode>/<javaRelativePath>),
    // so it inherently tracks the version the server asks for — a stale JRE from an older
    // launcher lives under a different jreCode dir and is simply never resolved. Download
    // only when that exact path is missing.
    const QString jreCode = ld.jreCode();
    QString javaPath = JavaDownloader::javaPathFor(jreCode, ld.jreFiles());
    const bool needJre = !javaPath.isEmpty() && !QFileInfo::exists(javaPath);
    qInfo().noquote() << "find: java path =" << (javaPath.isEmpty() ? QStringLiteral("(no match)") : javaPath)
                      << "manifest jreCode =" << jreCode << "needDownload =" << needJre;
    if (needJre) {
        JavaDownloader jd(&dl);
        jd.setCandidates(jreCode, ld.jreFiles());
        const QString dlPath = jd.download(this);
        if (!dlPath.isEmpty()) javaPath = dlPath;
    }
    resolvedJavaPath_ = javaPath;   // consumed by Launcher::run() after finished()

    qInfo() << "find: checking launcher.jar is up to date";
    if (!ld.checkFile()) ld.update(this);
    else                 qInfo() << "find: launcher.jar already up to date";
}

void Bootstrapper::run() {
    const bool ok = tryExponential(10, *this,
        [this]{ checkAndDownloadAll(); },
        [](int s){ QThread::sleep(s); });
    if (!ok) {
        qWarning() << "bootstrap failed after all retries (no network?); "
                      "will still attempt launch then exit";
        setStatus(QStringLiteral("Ошибка при загрузке. Проверьте подключение интернета"));
    }
    emit finished();
}

}
