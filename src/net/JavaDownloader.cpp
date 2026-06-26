#include "net/JavaDownloader.h"
#include "net/Downloader.h"
#include "app/ProgressMonitor.h"
#include "util/Platform.h"
#include "util/Paths.h"
#include "util/Archive.h"
#include <QDir>
#include <QObject>

namespace tprunner {

namespace { const char* kJresUrl = "https://minecraft.glitchless.ru/jres.json"; }

JavaDownloader::JavaDownloader(Downloader* dl) : dl_(dl) {}

std::optional<JavaBinaryModel> JavaDownloader::findMatch(
        const QList<JavaBinaryModel>& list, Os os, CpuArch arch) {
    for (const auto& m : list)
        if (osFromString(m.type) == os && archFromString(m.arch) == arch)
            return m;
    return std::nullopt;
}

void JavaDownloader::init() {
    const QByteArray json = dl_->httpGet(kJresUrl);
    const auto list = JavaBinaryModel::listFromJson(json);
    selected_ = findMatch(list, currentOs(), currentArch());
}

bool JavaDownloader::hasMatch() const { return selected_.has_value(); }

QString JavaDownloader::download(ProgressMonitor* monitor) {
    if (!selected_) return QString();
    const JavaBinaryModel& jb = *selected_;

    const QString jreArchive = Paths::temporaryDirectory() + "/jre." + jb.extension;
    if (monitor) monitor->setStatus(QObject::tr("Загрузка Java..."));
    dl_->downloadToFile(jb.downloadUrl, jreArchive, monitor);
    if (monitor) monitor->setProgress(100);

    if (jb.extension.compare("zip", Qt::CaseInsensitive) == 0)
        Archive::extractZip(jreArchive, Paths::javaDirectory());
    else
        Archive::extractTarGz(jreArchive, Paths::javaDirectory());

    return QDir(Paths::javaDirectory()).filePath(jb.javaRelativePath);
}

}
