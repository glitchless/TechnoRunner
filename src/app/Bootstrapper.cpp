#include "app/Bootstrapper.h"
#include "net/Downloader.h"
#include "net/JavaDownloader.h"
#include "net/LauncherDownloader.h"
#include "util/Paths.h"
#include <QNetworkAccessManager>
#include <QFile>
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

    const QString jrePathFile = Paths::jrePathFile();
    QFile jf(jrePathFile);
    QString existing;
    if (jf.exists() && jf.open(QIODevice::ReadOnly)) { existing = QString::fromUtf8(jf.readAll()); jf.close(); }
    const bool needJre = existing.isEmpty() || !QFileInfo::exists(existing);
    if (needJre) {
        JavaDownloader jd(&dl);
        jd.init();
        const QString javaPath = jd.download(this);
        if (!javaPath.isEmpty()) Paths::writeJrePath(javaPath);
    }

    LauncherDownloader ld(&dl);
    ld.init();
    if (!ld.checkFile()) ld.update(this);
}

void Bootstrapper::run() {
    const bool ok = tryExponential(10, *this,
        [this]{ checkAndDownloadAll(); },
        [](int s){ QThread::sleep(s); });
    if (!ok)
        setStatus(QStringLiteral("Ошибка при загрузке. Проверьте подключение интернета"));
    emit finished();
}

}
