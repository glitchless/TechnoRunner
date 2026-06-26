#include <QtTest>
#include "models/Models.h"

using namespace tprunner;

// NOTE: plain escaped string literals (not R"(...)") on purpose — moc mis-parses
// raw string literals containing "//" and braces and emits an empty meta-object.

class ModelsTest : public QObject {
    Q_OBJECT
private slots:
    void parsesJresArray() {
        const QByteArray json =
            "[{\"type\":\"Linux\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\","
            "\"downloadUrl\":\"https://x/l.tar.gz\",\"javaRelativePath\":\"jre/bin/java\"},"
            "{\"type\":\"macOS\",\"arch\":\"arm\",\"extension\":\"zip\","
            "\"downloadUrl\":\"https://x/m.zip\",\"javaRelativePath\":\"jre.jre/bin/java\"}]";
        const auto list = JavaBinaryModel::listFromJson(json);
        QCOMPARE(list.size(), 2);
        QCOMPARE(list[0].type, QStringLiteral("Linux"));
        QCOMPARE(list[0].extension, QStringLiteral("tar.gz"));
        QCOMPARE(list[1].arch, QStringLiteral("arm"));
        QCOMPARE(list[1].javaRelativePath, QStringLiteral("jre.jre/bin/java"));
    }
    void parsesLauncherObjectIncludingDashedKey() {
        const QByteArray json =
            "{\"version\":\"1.0.15\","
            "\"downloadFullPath\":\"https://minecraft.glitchless.ru/1.0.15.jar\","
            "\"SHA-256\":\"K6qafxioI7LGPcOeE8ZZrljmGenIfg860yPvjPV6JeM=\"}";
        const auto m = LauncherModel::fromJson(json);
        QVERIFY(m.has_value());
        QCOMPARE(m->version, QStringLiteral("1.0.15"));
        QCOMPARE(m->downloadUrl, QStringLiteral("https://minecraft.glitchless.ru/1.0.15.jar"));
        QCOMPARE(m->sha256, QStringLiteral("K6qafxioI7LGPcOeE8ZZrljmGenIfg860yPvjPV6JeM="));
    }
    void invalidLauncherJsonReturnsNullopt() {
        QVERIFY(!LauncherModel::fromJson("not json").has_value());
    }
    void parsesEmbeddedJreBlock() {
        const QByteArray json =
            "{\"version\":\"1.2.06221630\","
            "\"downloadFullPath\":\"https://minecraft.glitchless.ru/1.2.06221630.jar\","
            "\"SHA-256\":\"Nw1NQPLhV1V7Xg06b+XK0yry2M6x0zyxFIOHxvPM0uM=\","
            "\"jre\":{\"code\":\"jre8_202\",\"files\":["
            "{\"type\":\"Linux\",\"arch\":\"x86\",\"extension\":\"tar.gz\",\"downloadUrl\":\"https://x/li586.tar.gz\",\"javaRelativePath\":\"jre1.8.0_202/bin/java\"},"
            "{\"type\":\"Linux\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"https://x/lx64.tar.gz\",\"javaRelativePath\":\"jre1.8.0_202/bin/java\",\"SHA-256\":\"bSQf1wzJSvZnozyE7F+H0CthgVsIHSVcqPhaG233LUw=\",\"javaSHA-256\":\"RWIKRot34W0afUbhtHJEN/0lC6XYsuxCJBOZ4x01uDs=\"},"
            "{\"type\":\"Windows\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"https://x/wx64.tar.gz\",\"javaRelativePath\":\"jre1.8.0_202/bin/java.exe\"},"
            "{\"type\":\"macOS\",\"arch\":\"x86_64\",\"extension\":\"tar.gz\",\"downloadUrl\":\"https://x/mx64.tar.gz\",\"javaRelativePath\":\"jre1.8.0_202.jre/Contents/Home/bin/java\"}"
            "]}}";
        const auto m = LauncherModel::fromJson(json);
        QVERIFY(m.has_value());
        QCOMPARE(m->sha256, QStringLiteral("Nw1NQPLhV1V7Xg06b+XK0yry2M6x0zyxFIOHxvPM0uM="));
        QCOMPARE(m->jreCode, QStringLiteral("jre8_202"));
        QCOMPARE(m->jreFiles.size(), 4);
        QCOMPARE(m->jreFiles[1].type, QStringLiteral("Linux"));
        QCOMPARE(m->jreFiles[1].arch, QStringLiteral("x86_64"));
        QCOMPARE(m->jreFiles[1].extension, QStringLiteral("tar.gz"));
        QCOMPARE(m->jreFiles[1].sha256,
                 QStringLiteral("bSQf1wzJSvZnozyE7F+H0CthgVsIHSVcqPhaG233LUw="));
        QCOMPARE(m->jreFiles[1].javaSha256,
                 QStringLiteral("RWIKRot34W0afUbhtHJEN/0lC6XYsuxCJBOZ4x01uDs="));
        QCOMPARE(m->jreFiles[0].sha256, QString());     // absent → empty
        QCOMPARE(m->jreFiles[0].javaSha256, QString()); // absent → empty
        QCOMPARE(m->jreFiles[3].javaRelativePath,
                 QStringLiteral("jre1.8.0_202.jre/Contents/Home/bin/java"));
    }
    void oldLauncherFormatHasEmptyJre() { // backward tolerant
        const auto m = LauncherModel::fromJson(
            "{\"version\":\"1\",\"downloadFullPath\":\"u\",\"SHA-256\":\"h\"}");
        QVERIFY(m.has_value());
        QVERIFY(m->jreCode.isEmpty());
        QVERIFY(m->jreFiles.isEmpty());
        QVERIFY(m->files.isEmpty());
    }
    void parsesPerArchLauncherFiles() {
        const QByteArray json =
            "{\"version\":\"1\",\"downloadFullPath\":\"top\",\"SHA-256\":\"toph\",\"files\":["
            "{\"type\":\"Linux\",\"arch\":\"x86_64\",\"downloadUrl\":\"lj\",\"SHA-256\":\"ljh\"},"
            "{\"type\":\"macOS\",\"arch\":\"arm64\",\"downloadUrl\":\"mj\",\"SHA-256\":\"mjh\"}]}";
        const auto m = LauncherModel::fromJson(json);
        QVERIFY(m.has_value());
        QCOMPARE(m->downloadUrl, QStringLiteral("top"));  // single fallback still parsed
        QCOMPARE(m->files.size(), 2);
        QCOMPARE(m->files[0].type, QStringLiteral("Linux"));
        QCOMPARE(m->files[0].downloadUrl, QStringLiteral("lj"));
        QCOMPARE(m->files[0].sha256, QStringLiteral("ljh"));
    }
};

QTEST_APPLESS_MAIN(ModelsTest)
#include "ModelsTest.moc"
