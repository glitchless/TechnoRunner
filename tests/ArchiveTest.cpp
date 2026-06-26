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
