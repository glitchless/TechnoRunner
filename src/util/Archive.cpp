#include "util/Archive.h"
#include <QDir>
#include <QByteArray>
#include <QString>
#include <QtGlobal>
#include <archive.h>
#include <archive_entry.h>
#include <stdexcept>
#include <string>

namespace tprunner {

namespace {

void copyData(struct archive* in, struct archive* out) {
    const void* buff; size_t size; la_int64_t offset;
    for (;;) {
        int r = archive_read_data_block(in, &buff, &size, &offset);
        if (r == ARCHIVE_EOF) return;
        if (r < ARCHIVE_OK) throw std::runtime_error(archive_error_string(in));
        if (archive_write_data_block(out, buff, size, offset) < ARCHIVE_OK)
            throw std::runtime_error(archive_error_string(out));
    }
}

void extractTo(const QString& archivePath, const QString& destDir) {
    QDir().mkpath(destDir);
    struct archive* a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    struct archive* ext = archive_write_disk_new();
    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
        ARCHIVE_EXTRACT_ACL  | ARCHIVE_EXTRACT_FFLAGS |
        // Refuse ".." path components and writes through symlinks: the archive is
        // fetched over the network, so a malicious/MITM'd JRE must not escape destDir.
        ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_SECURE_SYMLINKS);
    archive_write_disk_set_standard_lookup(ext);

    // Open the archive. On Windows the wide-char (_w) entry points are required: the
    // narrow archive_read_open_filename treats the path bytes as the system ANSI
    // codepage, so a non-ASCII install path (e.g. a Cyrillic Windows username) fails to
    // open. POSIX filesystems are UTF-8, so the narrow path is correct there.
#ifdef Q_OS_WIN
    const std::wstring wsrc = archivePath.toStdWString();
    const int openRc = archive_read_open_filename_w(a, wsrc.c_str(), 10240);
#else
    const QByteArray src = archivePath.toUtf8();
    const int openRc = archive_read_open_filename(a, src.constData(), 10240);
#endif
    if (openRc != ARCHIVE_OK) {
        const std::string err = archive_error_string(a) ? archive_error_string(a) : "open failed";
        archive_read_free(a); archive_write_free(ext);
        throw std::runtime_error(err);
    }

    try {
        struct archive_entry* entry;
        int r;
        while ((r = archive_read_next_header(a, &entry)) != ARCHIVE_EOF) {
            if (r < ARCHIVE_OK) throw std::runtime_error(archive_error_string(a));
            // Rebase each entry under destDir. Set the destination via the wide API on
            // Windows so a Unicode destDir round-trips into archive_write_disk; the entry
            // name itself is read wide too (archive_entry_pathname_w) for non-ASCII names.
#ifdef Q_OS_WIN
            const wchar_t* wname = archive_entry_pathname_w(entry);
            const QString entryName = wname ? QString::fromWCharArray(wname)
                                            : QString::fromUtf8(archive_entry_pathname(entry));
            const std::wstring wfull = (destDir + "/" + entryName).toStdWString();
            archive_entry_copy_pathname_w(entry, wfull.c_str());
#else
            const QByteArray full = (destDir + "/").toUtf8() + archive_entry_pathname(entry);
            archive_entry_set_pathname(entry, full.constData());
#endif
            if (archive_write_header(ext, entry) < ARCHIVE_OK)
                throw std::runtime_error(archive_error_string(ext));
            if (archive_entry_size(entry) > 0)
                copyData(a, ext);
            archive_write_finish_entry(ext);
        }
    } catch (...) {
        archive_read_free(a); archive_write_free(ext);
        throw;
    }
    archive_read_free(a);
    archive_write_free(ext);
}

} // namespace

void Archive::extractZip(const QString& archivePath, const QString& destDir)   { extractTo(archivePath, destDir); }
void Archive::extractTarGz(const QString& archivePath, const QString& destDir) { extractTo(archivePath, destDir); }

}
