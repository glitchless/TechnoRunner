#include "net/JavaDownloader.h"
#include "net/Downloader.h"
#include "app/ProgressMonitor.h"
#include "util/Platform.h"
#include "util/Paths.h"
#include "util/Archive.h"
#include <QDir>
#include <QObject>

namespace tprunner {

JavaDownloader::JavaDownloader(Downloader* dl) : dl_(dl) {}

std::optional<JavaBinaryModel> JavaDownloader::findMatch(
        const QList<JavaBinaryModel>& list, Os os, CpuArch arch) {
    for (const auto& m : list)
        if (osFromString(m.type) == os && archFromString(m.arch) == arch)
            return m;
    return std::nullopt;
}

void JavaDownloader::setCandidates(const QString& code, const QList<JavaBinaryModel>& files) {
    code_ = code;
    selected_ = findMatch(files, currentOs(), currentArch());
}

bool JavaDownloader::hasMatch() const { return selected_.has_value(); }

QString JavaDownloader::download(ProgressMonitor* monitor) {
    if (!selected_) return QString();
    const JavaBinaryModel& jb = *selected_;

    const QString destDir = Paths::javaDirectory(code_);   // jre/<code>
    const QString jreArchive = Paths::temporaryDirectory() + "/jre." + jb.extension;
    if (monitor) monitor->setStatus(QObject::tr("Загрузка Java..."));
    dl_->downloadToFile(jb.downloadUrl, jreArchive, monitor);
    if (monitor) monitor->setProgress(100);

    if (jb.extension.compare("zip", Qt::CaseInsensitive) == 0)
        Archive::extractZip(jreArchive, destDir);
    else
        Archive::extractTarGz(jreArchive, destDir);

    return QDir(destDir).filePath(jb.javaRelativePath);
}

}
