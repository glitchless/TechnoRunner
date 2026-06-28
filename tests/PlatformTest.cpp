#include <QtTest>
#include "util/Platform.h"

using namespace tprunner;

class PlatformTest : public QObject {
    Q_OBJECT
private slots:
    void osFromStringMatchesOslib() {
        QCOMPARE(osFromString("Linux"),   Os::Linux);
        QCOMPARE(osFromString("linux"),   Os::Linux);
        QCOMPARE(osFromString("Windows"), Os::Windows);
        QCOMPARE(osFromString("macOS"),   Os::MacOs);
        QCOMPARE(osFromString("mac os x"),Os::MacOs);   // contains "mac"
    }
    void archFromStringMatchesOslib() {
        QCOMPARE(archFromString("x86_64"), CpuArch::X86_64);
        QCOMPARE(archFromString("amd64"),  CpuArch::X86_64);
        QCOMPARE(archFromString("arm"),    CpuArch::Arm);   // matches "ARM" case-insensitively
        QCOMPARE(archFromString("arm64"),  CpuArch::Arm);
        QCOMPARE(archFromString("i686"),   CpuArch::X86);
        QCOMPARE(archFromString("aarch64"),CpuArch::Unknown); // not in any oslib list
    }
    void workingDirectoryPerOs() {
        QCOMPARE(workingDirectoryForOs(Os::Linux, "/home/u", "/ignored"),
                 QStringLiteral("/home/u/.minecraft"));
        QCOMPARE(workingDirectoryForOs(Os::MacOs, "/Users/u", "/ignored"),
                 QStringLiteral("/Users/u/Library/Application Support/minecraft"));
        QCOMPARE(workingDirectoryForOs(Os::Windows, "C:/Users/u", "C:/Users/u/AppData/Roaming"),
                 QStringLiteral("C:/Users/u/AppData/Roaming/.minecraft"));
        QCOMPARE(workingDirectoryForOs(Os::Windows, "C:/Users/u", ""),  // APPDATA unset → home
                 QStringLiteral("C:/Users/u/.minecraft"));
    }
    void currentOsIsKnown() {
        QVERIFY(currentOs() != Os::Unknown);
    }
};

QTEST_APPLESS_MAIN(PlatformTest)
#include "PlatformTest.moc"
