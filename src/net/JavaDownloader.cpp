#include "net/JavaDownloader.h"
#include "net/Downloader.h"
#include "app/ProgressMonitor.h"
#include "util/Platform.h"
#include "util/Paths.h"
#include "util/Archive.h"
#include "util/Hash.h"
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QDebug>
#include <stdexcept>

namespace tprunner {

JavaDownloader::JavaDownloader(Downloader* dl) : dl_(dl) {}

std::optional<JavaBinaryModel> JavaDownloader::findMatch(
        const QList<JavaBinaryModel>& list, Os os, CpuArch arch) {
    return matchBinary(list, os, arch);
}

void JavaDownloader::setCandidates(const QString& code, const QList<JavaBinaryModel>& files) {
    code_ = code;
    selected_ = findMatch(files, currentOs(), currentArch());
    if (selected_)
        qInfo().noquote() << "find: JRE match for this platform — code" << code
                          << "url" << selected_->downloadUrl;
    else
        qWarning().noquote() << "find: NO JRE match for this platform among"
                             << files.size() << "candidates (code" << code << ")";
}

bool JavaDownloader::hasMatch() const { return selected_.has_value(); }

QString JavaDownloader::javaPathFor(const QString& code, const QList<JavaBinaryModel>& files) {
    if (code.isEmpty()) return QString();
    const auto m = selectForCurrentPlatform(files);
    if (!m) return QString();
    // cleanPath (not Paths::javaDirectory) so this stays side-effect-free — javaDirectory
    // would mkpath the jre/<code> dir just for a check.
    return QDir::cleanPath(Paths::baseDirectory() + "/jre/" + code + "/" + m->javaRelativePath);
}

void JavaDownloader::checkFileHash(const QString& path, const QString& expectedSha256Base64) {
    if (expectedSha256Base64.isEmpty()) {
        qInfo().noquote() << "verify: skip (no expected hash in manifest) for" << path;
        return;   // manifest carries no hash → skip
    }
    const QString actual = sha256Base64(path);
    if (actual != expectedSha256Base64) {
        qWarning().noquote() << "verify: FAIL" << path
                             << "expected" << expectedSha256Base64 << "got" << actual;
        throw std::runtime_error(
            ("hash mismatch for " + path +
             " (expected " + expectedSha256Base64 + ", got " + actual + ")").toStdString());
    }
    qInfo().noquote() << "verify: ok" << path;
}

QString JavaDownloader::download(ProgressMonitor* monitor) {
    if (!selected_) return QString();
    const JavaBinaryModel& jb = *selected_;

    const QString destDir = Paths::javaDirectory(code_);   // jre/<code>
    const QString jreArchive = Paths::temporaryDirectory() + "/jre." + jb.extension;
    if (monitor) monitor->setStatus(QObject::tr("Загрузка Java..."));
    dl_->downloadToFile(jb.downloadUrl, jreArchive, monitor);
    if (monitor) monitor->setProgress(100);

    checkFileHash(jreArchive, jb.sha256);   // verify the download before extracting

    qInfo().noquote() << "download: extracting" << jreArchive << "->" << destDir
                      << "(" << jb.extension << ")";
    if (jb.extension.compare("zip", Qt::CaseInsensitive) == 0)
        Archive::extractZip(jreArchive, destDir);
    else
        Archive::extractTarGz(jreArchive, destDir);

    // Verify the extraction actually produced the java binary the manifest promised...
    const QString javaPath = QDir(destDir).filePath(jb.javaRelativePath);
    if (!QFileInfo::exists(javaPath)) {
        qWarning().noquote() << "find: JRE extracted but java NOT found at" << javaPath;
        throw std::runtime_error(("JRE extracted but java not found at " + javaPath).toStdString());
    }
    qInfo().noquote() << "find: java binary at" << javaPath;
    // ...and that the extracted binary matches its expected hash.
    checkFileHash(javaPath, jb.javaSha256);
    qInfo().noquote() << "find: JRE ready" << javaPath;
    return javaPath;
}

}
