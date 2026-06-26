#include <QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include "net/JavaDownloader.h"
#include "util/Platform.h"
#include "util/Paths.h"

using namespace tprunner;

// Plain escaped JSON (not R"(...)") — moc mis-parses raw string literals.

class JavaDownloaderTest : public QObject {
    Q_OBJECT
    QList<JavaBinaryModel> list_;
    QTemporaryDir tmp_;
private slots:
    void init()    { Paths::setBaseOverride(tmp_.path()); }
    void cleanup() { Paths::setBaseOverride(QString()); }

    void initTestCase() {
        list_ = JavaBinaryModel::listFromJson(
            "[{\"type\":\"Linux\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u1\",\"javaRelativePath\":\"p1\"},"
            "{\"type\":\"macOS\",\"arch\":\"x86_64\",\"extension\":\"zip\",\"downloadUrl\":\"u2\",\"javaRelativePath\":\"p2\"},"
            "{\"type\":\"macOS\",\"arch\":\"arm\",\"extension\":\"zip\",\"downloadUrl\":\"u3\",\"javaRelativePath\":\"p3\"}]");
    }
    void matchesLinuxX64() {
        const auto m = JavaDownloader::findMatch(list_, Os::Linux, CpuArch::X86_64);
        QVERIFY(m.has_value()); QCOMPARE(m->downloadUrl, QStringLiteral("u1"));
    }
    void matchesMacArmDistinctFromMacX64() {
        const auto m = JavaDownloader::findMatch(list_, Os::MacOs, CpuArch::Arm);
        QVERIFY(m.has_value()); QCOMPARE(m->javaRelativePath, QStringLiteral("p3"));
    }
    void noMatchReturnsNullopt() {
        const auto m = JavaDownloader::findMatch(list_, Os::Windows, CpuArch::X86_64);
        QVERIFY(!m.has_value());
    }
    void setCandidatesMatchesCurrentMachine() {
        // Candidates covering every CI runner (linux/win x64, macOS x64/arm) → always a match.
        const auto files = JavaBinaryModel::listFromJson(
            "[{\"type\":\"Linux\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"p\"},"
            "{\"type\":\"Windows\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"p\"},"
            "{\"type\":\"macOS\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"p\"},"
            "{\"type\":\"macOS\",\"arch\":\"arm\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"p\"}]");
        JavaDownloader jd(nullptr);
        jd.setCandidates(QStringLiteral("jre8_202"), files);
        QVERIFY(jd.hasMatch());
    }
    void setCandidatesEmptyHasNoMatch() {
        JavaDownloader jd(nullptr);
        jd.setCandidates(QStringLiteral("jre8_202"), {});
        QVERIFY(!jd.hasMatch());
    }
    void hashCheckPassesOnMatch() {
        QTemporaryFile f; QVERIFY(f.open()); f.write("abc"); f.close();
        // SHA-256("abc") Base64 — must not throw
        JavaDownloader::checkFileHash(f.fileName(),
            QStringLiteral("ungWv48Bz+pBQUDeXa4iI7ADYaOWF3qctBD/YfIAFa0="));
    }
    void hashCheckThrowsOnMismatch() {
        QTemporaryFile f; QVERIFY(f.open()); f.write("abc"); f.close();
        QVERIFY_EXCEPTION_THROWN(
            JavaDownloader::checkFileHash(f.fileName(), QStringLiteral("wronghash=")),
            std::runtime_error);
    }
    void hashCheckSkippedWhenEmpty() {
        QTemporaryFile f; QVERIFY(f.open()); f.write("abc"); f.close();
        JavaDownloader::checkFileHash(f.fileName(), QString()); // no-op, no throw
    }

    // javaPathFor resolves <base>/jre/<code>/<javaRelativePath> for this machine.
    void javaPathForResolvesUnderJreCode() {
        // Candidates covering every CI runner so the current platform always matches.
        const auto files = JavaBinaryModel::listFromJson(
            "[{\"type\":\"Linux\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"bin/java\"},"
            "{\"type\":\"Windows\",\"arch\":\"x86_64\",\"extension\":\"zip\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"bin/java.exe\"},"
            "{\"type\":\"macOS\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"bin/java\"},"
            "{\"type\":\"macOS\",\"arch\":\"arm\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"bin/java\"}]");
        const QString p = JavaDownloader::javaPathFor(QStringLiteral("jre21"), files);
        QVERIFY2(p.startsWith(tmp_.path() + "/jre/jre21/"), qPrintable(p));
        QVERIFY(p.endsWith("/java") || p.endsWith("/java.exe"));
    }
    void javaPathForEmptyWhenNoMatchOrNoCode() {
        // No entry for this platform → empty.
        QVERIFY(JavaDownloader::javaPathFor(QStringLiteral("jre21"),
                JavaBinaryModel::listFromJson("[]")).isEmpty());
        // Empty jreCode → empty regardless of candidates.
        const auto any = JavaBinaryModel::listFromJson(
            "[{\"type\":\"Linux\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"bin/java\"},"
            "{\"type\":\"Windows\",\"arch\":\"x86_64\",\"extension\":\"zip\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"bin/java.exe\"},"
            "{\"type\":\"macOS\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"bin/java\"},"
            "{\"type\":\"macOS\",\"arch\":\"arm\",\"extension\":\"tar.gz\",\"downloadUrl\":\"u\",\"javaRelativePath\":\"bin/java\"}]");
        QVERIFY(JavaDownloader::javaPathFor(QString(), any).isEmpty());
    }
};

QTEST_APPLESS_MAIN(JavaDownloaderTest)
#include "JavaDownloaderTest.moc"
