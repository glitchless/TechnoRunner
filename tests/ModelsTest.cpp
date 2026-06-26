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
};

QTEST_APPLESS_MAIN(ModelsTest)
#include "ModelsTest.moc"
