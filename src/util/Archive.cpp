#include "util/Archive.h"
#include <QDir>
#include <QByteArray>
#include <archive.h>
#include <archive_entry.h>
#include <stdexcept>

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

    const QByteArray src = archivePath.toUtf8();
    if (archive_read_open_filename(a, src.constData(), 10240) != ARCHIVE_OK) {
        const std::string err = archive_error_string(a) ? archive_error_string(a) : "open failed";
        archive_read_free(a); archive_write_free(ext);
        throw std::runtime_error(err);
    }

    const QByteArray destPrefix = (destDir + "/").toUtf8();
    try {
        struct archive_entry* entry;
        int r;
        while ((r = archive_read_next_header(a, &entry)) != ARCHIVE_EOF) {
            if (r < ARCHIVE_OK) throw std::runtime_error(archive_error_string(a));
            const QByteArray full = destPrefix + archive_entry_pathname(entry);
            archive_entry_set_pathname(entry, full.constData());
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
