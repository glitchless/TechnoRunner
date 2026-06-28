#include <QtTest>
#include <QPixmap>
#include "ui/SplashScreen.h"

using namespace tprunner;

class SplashScreenTest : public QObject {
    Q_OBJECT
private slots:
    void resourcesAreEmbedded() {
        QVERIFY(!QPixmap(QStringLiteral(":/background.jpeg")).isNull());
        QVERIFY(!QPixmap(QStringLiteral(":/close-btn.png")).isNull());
    }
    void statusSlotUpdatesLabel() {
        SplashScreen s;
        s.onStatus("Проверка");
        QCOMPARE(s.statusText(), QStringLiteral("Проверка"));
    }
    void progressSlotUpdatesBar() {
        SplashScreen s;
        s.onMax(200);
        s.onProgress(50);
        QCOMPARE(s.progressValue(), 50);
    }
};

QTEST_MAIN(SplashScreenTest)
#include "SplashScreenTest.moc"
