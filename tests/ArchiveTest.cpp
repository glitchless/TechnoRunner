#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
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
};

QTEST_APPLESS_MAIN(ArchiveTest)
#include "ArchiveTest.moc"
