#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "util/Paths.h"

using namespace tprunner;

class PathsTest : public QObject {
    Q_OBJECT
    QTemporaryDir tmp_;
private slots:
    void init()    { Paths::setBaseOverride(tmp_.path()); }
    void cleanup() { Paths::setBaseOverride(QString()); }

    void subPathsLayout() {
        QCOMPARE(Paths::baseDirectory(), tmp_.path());
        QCOMPARE(Paths::temporaryDirectory(), tmp_.path() + "/tmp");
        QCOMPARE(Paths::javaDirectory(),      tmp_.path() + "/jre");
        QCOMPARE(Paths::manifestFile(),       tmp_.path() + "/launcher.json");
        QCOMPARE(Paths::launcherFile(),       tmp_.path() + "/launcher.jar");
        QVERIFY(QFileInfo::exists(tmp_.path() + "/tmp")); // created on demand
    }
    void javaDirectoryWithCode() {
        QCOMPARE(Paths::javaDirectory(QStringLiteral("jre8_202")),
                 tmp_.path() + "/jre/jre8_202");
        QVERIFY(QFileInfo::exists(tmp_.path() + "/jre/jre8_202")); // created on demand
    }
};

QTEST_APPLESS_MAIN(PathsTest)
#include "PathsTest.moc"
