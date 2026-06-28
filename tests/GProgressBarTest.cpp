#include <QtTest>
#include "ui/GProgressBar.h"

using namespace tprunner;

class GProgressBarTest : public QObject {
    Q_OBJECT
private slots:
    void hasFixedHeightAndAcceptsValue() {
        GProgressBar bar;
        bar.setRange(0, 100);
        bar.setValue(42);
        QCOMPARE(bar.value(), 42);
        QCOMPARE(bar.minimumHeight(), 20);
    }
};

QTEST_MAIN(GProgressBarTest)
#include "GProgressBarTest.moc"
