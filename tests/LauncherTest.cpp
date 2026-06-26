#include <QtTest>
#include "run/Launcher.h"

using namespace tprunner;

class LauncherTest : public QObject {
    Q_OBJECT
private slots:
    // java is launched directly (no shell): program is the java binary, arguments are
    // exactly {-jar, <jar>}, and the redirect targets are carried as data for QProcess's
    // own stdout/stderr channels — never a shell `>` baked into the arguments.
    void buildsDirectJavaCommand() {
        const auto c = Launcher::buildCommand("C:/jre/java.exe", "C:/d/launcher.jar",
                                              "C:/d/out.log", "C:/d/err.log");
        QCOMPARE(c.program, QStringLiteral("C:/jre/java.exe"));
        QCOMPARE(c.arguments, (QStringList{ QStringLiteral("-jar"), QStringLiteral("C:/d/launcher.jar") }));
        QCOMPARE(c.outLog, QStringLiteral("C:/d/out.log"));
        QCOMPARE(c.errLog, QStringLiteral("C:/d/err.log"));
    }

    // No shell metacharacters leak into the arguments: nothing routed through cmd.exe/sh,
    // so embedded spaces/quotes in paths are handled by QProcess, not re-quoting.
    void argumentsCarryNoRedirectOperators() {
        const auto c = Launcher::buildCommand("/jre/bin/java", "/d/launcher.jar",
                                              "/d/out.log", "/d/err.log");
        for (const QString& a : c.arguments) {
            QVERIFY(!a.contains('>'));
            QVERIFY(!a.contains("/C"));
            QVERIFY(!a.contains("-c"));
        }
    }
};

QTEST_APPLESS_MAIN(LauncherTest)
#include "LauncherTest.moc"
