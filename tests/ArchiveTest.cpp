#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include "util/Archive.h"

using namespace tprunner;

namespace {
QString fixture(const QString& name) {
    return QString(SRCDIR) + "/fixtures/" + name; // SRCDIR injected by CMake
}
QString readAll(const QString& path) {
    QFile f(path); f.open(QIODevice::ReadOnly); return QString::fromUtf8(f.readAll());
}
}

class ArchiveTest : public QObject {
    Q_OBJECT
private slots:
    void extractsTarGz() {
        QTemporaryDir dir;
        Archive::extractTarGz(fixture("sample.tar.gz"), dir.path());
        QCOMPARE(readAll(dir.path() + "/a.txt"), QStringLiteral("hello-tar"));
        QCOMPARE(readAll(dir.path() + "/sub/b.txt"), QStringLiteral("nested"));
    }
    void extractsZip() {
        QTemporaryDir dir;
        Archive::extractZip(fixture("sample.zip"), dir.path());
        QCOMPARE(readAll(dir.path() + "/a.txt"), QStringLiteral("hello-tar"));
        QCOMPARE(readAll(dir.path() + "/sub/b.txt"), QStringLiteral("nested"));
    }
    // Non-ASCII archive + destination paths (e.g. a Cyrillic Windows username like
    // C:\Users\<cyrillic>\...). Regression: libarchive's narrow API mis-decoded UTF-8
    // paths on Windows, so extraction failed after the download completed. fromUtf8 with
    // \x byte escapes keeps the test independent of the source file's encoding / compiler
    // charset flags (the bytes below are UTF-8 for Cyrillic "тест": т е с т).
    void extractsWithUnicodePaths() {
        const QString cyr = QString::fromUtf8("\xD1\x82\xD0\xB5\xD1\x81\xD1\x82");
        QTemporaryDir dir;
        const QString srcZip = dir.path() + "/" + cyr + ".zip";
        QVERIFY(QFile::copy(fixture("sample.zip"), srcZip));   // Cyrillic SOURCE path
        const QString dest = dir.path() + "/" + cyr + "-out";  // Cyrillic DEST path
        Archive::extractZip(srcZip, dest);
        QCOMPARE(readAll(dest + "/a.txt"), QStringLiteral("hello-tar"));
        QCOMPARE(readAll(dest + "/sub/b.txt"), QStringLiteral("nested"));
    }
    void missingArchiveThrows() {
        QTemporaryDir dir;
        QVERIFY_EXCEPTION_THROWN(Archive::extractZip("/no/such.zip", dir.path()), std::runtime_error);
    }
    void rejectsPathTraversal() { // Zip Slip: an entry named "../escaped.txt" must not escape destDir
        QTemporaryDir dir;
        const QString dest = dir.path() + "/out";
        QDir().mkpath(dest);
        QVERIFY_EXCEPTION_THROWN(Archive::extractZip(fixture("evil.zip"), dest), std::runtime_error);
        QVERIFY(!QFileInfo::exists(dir.path() + "/escaped.txt"));
    }
};

QTEST_APPLESS_MAIN(ArchiveTest)
#include "ArchiveTest.moc"
