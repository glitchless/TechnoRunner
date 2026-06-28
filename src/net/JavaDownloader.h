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
    // Absolute path the current platform's JRE would have once installed:
    // <base>/jre/<code>/<javaRelativePath>. "" if `code` is empty or no entry matches
    // this machine. Pure (does not create directories or touch the network) — used both
    // to decide whether a download is needed and to resolve the java to launch.
    static QString javaPathFor(const QString& code, const QList<JavaBinaryModel>& files);
    // Throws std::runtime_error if the file's Base64 SHA-256 != expected. No-op when
    // expected is empty (manifest without a hash). Used for both the downloaded archive
    // and the extracted java binary.
    static void checkFileHash(const QString& path, const QString& expectedSha256Base64);
private:
    Downloader* dl_;
    QString code_;
    std::optional<JavaBinaryModel> selected_;
};

}
