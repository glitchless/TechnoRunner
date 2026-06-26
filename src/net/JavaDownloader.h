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
    // Configure from the launcher manifest's "jre" block: `code` names the install
    // subfolder (jre/<code>); `files` are the per-OS/arch candidates.
    void setCandidates(const QString& code, const QList<JavaBinaryModel>& files);
    bool hasMatch() const;
    QString download(ProgressMonitor* monitor);   // extracts to jre/<code>, returns java path or "" if no match
    static std::optional<JavaBinaryModel> findMatch(const QList<JavaBinaryModel>& list,
                                                    Os os, CpuArch arch);
    // Throws std::runtime_error if the file's Base64 SHA-256 != expected. No-op when
    // expected is empty (manifest without a hash).
    static void checkArchiveHash(const QString& path, const QString& expectedSha256Base64);
private:
    Downloader* dl_;
    QString code_;
    std::optional<JavaBinaryModel> selected_;
};

}
