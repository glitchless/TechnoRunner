#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "net/LauncherDownloader.h"
#include "util/Paths.h"
#include "util/Hash.h"

using namespace tprunner;

namespace {
// Subclass that injects a manifest body instead of hitting the network.
class FakeLauncherDownloader : public LauncherDownloader {
public:
    QByteArray manifest;
    explicit FakeLauncherDownloader() : LauncherDownloader(nullptr) {}
protected:
    QByteArray fetchManifest() override { return manifest; }
};
void writeFile(const QString& path, const QByteArray& data) {
    QFile f(path); f.open(QIODevice::WriteOnly); f.write(data); f.close();
}
}

class LauncherDownloaderTest : public QObject {
    Q_OBJECT
    QTemporaryDir tmp_;
private slots:
    void init()    { Paths::setBaseOverride(tmp_.path()); }
    void cleanup() { Paths::setBaseOverride(QString()); }

    void checkFileFalseWhenMissing() {
        FakeLauncherDownloader d; d.manifest = "{}"; d.init();
        QVERIFY(!d.checkFile());
    }
    void checkFileTrueWhenHashMatches() {
        writeFile(Paths::launcherFile(), "JARBYTES");
        const QString h = sha256Base64(Paths::launcherFile());
        FakeLauncherDownloader d;
        d.manifest = QByteArray("{\"version\":\"1\",\"downloadFullPath\":\"u\",\"SHA-256\":\"") + h.toUtf8() + "\"}";
        d.init();
        QVERIFY(d.checkFile());
    }
    void checkFileFalseWhenHashDiffers() {
        writeFile(Paths::launcherFile(), "JARBYTES");
        FakeLauncherDownloader d;
        d.manifest = "{\"version\":\"1\",\"downloadFullPath\":\"u\",\"SHA-256\":\"deadbeef\"}";
        d.init();
        QVERIFY(!d.checkFile());
    }
    void checkFileTrueWhenNoModel() { // file exists, manifest invalid → assume OK (matches original)
        writeFile(Paths::launcherFile(), "JARBYTES");
        FakeLauncherDownloader d; d.manifest = "not json"; d.init();
        QVERIFY(d.checkFile());
    }
};

QTEST_APPLESS_MAIN(LauncherDownloaderTest)
#include "LauncherDownloaderTest.moc"
