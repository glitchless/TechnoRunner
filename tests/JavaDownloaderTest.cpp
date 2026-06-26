#include <QtTest>
#include "net/JavaDownloader.h"
#include "util/Platform.h"

using namespace tprunner;

// Plain escaped JSON (not R"(...)") — moc mis-parses raw string literals.

class JavaDownloaderTest : public QObject {
    Q_OBJECT
    QList<JavaBinaryModel> list_;
private slots:
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
};

QTEST_APPLESS_MAIN(JavaDownloaderTest)
#include "JavaDownloaderTest.moc"
