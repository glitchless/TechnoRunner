#pragma once
#include <QString>
#include <optional>
#include "models/Models.h"

namespace tprunner {

class Downloader;
class ProgressMonitor;
enum class Os;
enum class CpuArch;

class JavaDownloader {
public:
    explicit JavaDownloader(Downloader* dl);
    void init();
    bool hasMatch() const;
    QString download(ProgressMonitor* monitor);   // returns absolute java path, or "" if no match
    static std::optional<JavaBinaryModel> findMatch(const QList<JavaBinaryModel>& list,
                                                    Os os, CpuArch arch);
private:
    Downloader* dl_;
    std::optional<JavaBinaryModel> selected_;
};

}
