#include <QtTest>
#include "app/Bootstrapper.h"
#include "app/ProgressMonitor.h"

using namespace tprunner;

namespace {
class NoopMonitor : public ProgressMonitor {
public:
    int statusCalls = 0;
    void setProgress(int) override {}
    void setMax(int) override {}
    void incrementProgress(int) override {}
    void setStatus(const QString&) override { ++statusCalls; }
};
}

class BootstrapperTest : public QObject {
    Q_OBJECT
private slots:
    void succeedsFirstTryNoSleep() {
        NoopMonitor mon; int sleeps = 0; int runs = 0;
        const bool ok = tryExponential(10, mon, [&]{ ++runs; },
                                       [&](int){ ++sleeps; });
        QVERIFY(ok); QCOMPARE(runs, 1); QCOMPARE(sleeps, 0);
    }
    void retriesThenSucceeds() {
        NoopMonitor mon; int sleeps = 0; int runs = 0;
        const bool ok = tryExponential(10, mon, [&]{
            if (++runs < 3) throw std::runtime_error("boom");
        }, [&](int){ ++sleeps; });
        QVERIFY(ok); QCOMPARE(runs, 3);
        // backoff seconds after attempt 0 (=1) and attempt 1 (=2) → 1 + 2 = 3 one-second sleeps
        QCOMPARE(sleeps, 3);
    }
    void returnsFalseAfterAllAttempts() {
        NoopMonitor mon; int runs = 0;
        const bool ok = tryExponential(3, mon, [&]{ ++runs; throw std::runtime_error("x"); },
                                       [](int){});
        QVERIFY(!ok); QCOMPARE(runs, 3);
    }
};

QTEST_APPLESS_MAIN(BootstrapperTest)
#include "BootstrapperTest.moc"
