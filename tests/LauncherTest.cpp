#include <QtTest>
#include "run/Launcher.h"
#include "util/Platform.h"

using namespace tprunner;

class LauncherTest : public QObject {
    Q_OBJECT
private slots:
    void buildsUnixShellRedirect() {
        const auto c = Launcher::buildCommand("/jre/bin/java", "/d/launcher.jar",
                                              "/d/out.log", "/d/err.log", Os::Linux);
        QCOMPARE(c.program, QStringLiteral("/bin/sh"));
        QCOMPARE(c.arguments.size(), 2);
        QCOMPARE(c.arguments[0], QStringLiteral("-c"));
        QVERIFY(c.arguments[1].contains("'/jre/bin/java' -jar '/d/launcher.jar'"));
        QVERIFY(c.arguments[1].contains("> '/d/out.log'"));
        QVERIFY(c.arguments[1].contains("2> '/d/err.log'"));
    }
    void buildsWindowsCmdRedirect() {
        const auto c = Launcher::buildCommand("C:/jre/java.exe", "C:/d/launcher.jar",
                                              "C:/d/out.log", "C:/d/err.log", Os::Windows);
        QCOMPARE(c.program, QStringLiteral("cmd.exe"));
        QCOMPARE(c.arguments[0], QStringLiteral("/C"));
        QVERIFY(c.arguments[1].contains("\"C:/jre/java.exe\" -jar \"C:/d/launcher.jar\""));
        QVERIFY(c.arguments[1].contains("> \"C:/d/out.log\""));
        QVERIFY(c.arguments[1].contains("2> \"C:/d/err.log\""));
    }
};

QTEST_APPLESS_MAIN(LauncherTest)
#include "LauncherTest.moc"
