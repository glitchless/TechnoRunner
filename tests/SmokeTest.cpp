#include <QtTest>

class SmokeTest : public QObject {
    Q_OBJECT
private slots:
    void arithmeticHolds() { QCOMPARE(2 + 2, 4); }
};

QTEST_APPLESS_MAIN(SmokeTest)
#include "SmokeTest.moc"
