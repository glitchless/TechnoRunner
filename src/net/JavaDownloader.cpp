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
}

bool JavaDownloader::hasMatch() const { return selected_.has_value(); }

void JavaDownloader::checkFileHash(const QString& path, const QString& expectedSha256Base64) {
    if (expectedSha256Base64.isEmpty()) return;   // manifest carries no hash → skip
    const QString actual = sha256Base64(path);
    if (actual != expectedSha256Base64)
        throw std::runtime_error(
            ("hash mismatch for " + path +
             " (expected " + expectedSha256Base64 + ", got " + actual + ")").toStdString());
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

    if (jb.extension.compare("zip", Qt::CaseInsensitive) == 0)
        Archive::extractZip(jreArchive, destDir);
    else
        Archive::extractTarGz(jreArchive, destDir);

    // Verify the extraction actually produced the java binary the manifest promised...
    const QString javaPath = QDir(destDir).filePath(jb.javaRelativePath);
    if (!QFileInfo::exists(javaPath))
        throw std::runtime_error(("JRE extracted but java not found at " + javaPath).toStdString());
    // ...and that the extracted binary matches its expected hash.
    checkFileHash(javaPath, jb.javaSha256);
    return javaPath;
}

}
