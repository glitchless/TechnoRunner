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
    void exposesEmbeddedJre() {
        FakeLauncherDownloader d;
        d.manifest =
            "{\"version\":\"1.2\",\"downloadFullPath\":\"u\",\"SHA-256\":\"h\","
            "\"jre\":{\"code\":\"jre8_202\",\"files\":["
            "{\"type\":\"Linux\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"lu\",\"javaRelativePath\":\"lp\"}"
            "]}}";
        d.init();
        QCOMPARE(d.jreCode(), QStringLiteral("jre8_202"));
        QCOMPARE(d.jreFiles().size(), 1);
        QCOMPARE(d.jreFiles()[0].downloadUrl, QStringLiteral("lu"));
    }
    void jreEmptyWhenNoModel() {
        FakeLauncherDownloader d; d.manifest = "not json"; d.init();
        QVERIFY(d.jreCode().isEmpty());
        QVERIFY(d.jreFiles().isEmpty());
    }
    void checkFileUsesMatchingPerArchHash() {
        writeFile(Paths::launcherFile(), "JARBYTES");
        const QString h = sha256Base64(Paths::launcherFile());
        auto e = [&](const char* t, const char* a) {
            return QString("{\"type\":\"%1\",\"arch\":\"%2\",\"downloadUrl\":\"u\",\"SHA-256\":\"%3\"}")
                .arg(QString(t), QString(a), h);
        };
        const QString files = e("Linux","x86_64") + "," + e("Linux","arm64") + "," +
                              e("Windows","x86_64") + "," + e("macOS","x86_64") + "," + e("macOS","arm64");
        FakeLauncherDownloader d;
        // top-level hash is deliberately wrong: only the per-arch match makes checkFile pass
        d.manifest = QString("{\"version\":\"1\",\"downloadFullPath\":\"u\",\"SHA-256\":\"WRONG\",\"files\":[%1]}")
                         .arg(files).toUtf8();
        d.init();
        QVERIFY(d.checkFile());
    }
    void checkFileFalseWhenPerArchHashDiffers() {
        writeFile(Paths::launcherFile(), "JARBYTES");
        FakeLauncherDownloader d;
        d.manifest =
            "{\"version\":\"1\",\"downloadFullPath\":\"u\",\"SHA-256\":\"toplevel\",\"files\":["
            "{\"type\":\"Linux\",\"arch\":\"x86_64\",\"downloadUrl\":\"u\",\"SHA-256\":\"nope\"},"
            "{\"type\":\"Linux\",\"arch\":\"arm64\",\"downloadUrl\":\"u\",\"SHA-256\":\"nope\"},"
            "{\"type\":\"Windows\",\"arch\":\"x86_64\",\"downloadUrl\":\"u\",\"SHA-256\":\"nope\"},"
            "{\"type\":\"macOS\",\"arch\":\"x86_64\",\"downloadUrl\":\"u\",\"SHA-256\":\"nope\"},"
            "{\"type\":\"macOS\",\"arch\":\"arm64\",\"downloadUrl\":\"u\",\"SHA-256\":\"nope\"}]}";
        d.init();
        QVERIFY(!d.checkFile());
    }
};

QTEST_APPLESS_MAIN(LauncherDownloaderTest)
#include "LauncherDownloaderTest.moc"
