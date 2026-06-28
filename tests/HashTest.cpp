#include <QtTest>
#include <QTemporaryFile>
#include "util/Hash.h"

class HashTest : public QObject {
    Q_OBJECT
private slots:
    void emptyFileHasKnownHash() {
        QTemporaryFile f; QVERIFY(f.open()); f.close();
        // SHA-256 of empty input, Base64-encoded (matches Java Base64.getEncoder()):
        QCOMPARE(tprunner::sha256Base64(f.fileName()),
                 QStringLiteral("47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU="));
    }
    void knownContentHash() {
        QTemporaryFile f; QVERIFY(f.open()); f.write("abc"); f.close();
        // SHA-256("abc") Base64:
        QCOMPARE(tprunner::sha256Base64(f.fileName()),
                 QStringLiteral("ungWv48Bz+pBQUDeXa4iI7ADYaOWF3qctBD/YfIAFa0="));
    }
    void missingFileThrows() {
        QVERIFY_EXCEPTION_THROWN(tprunner::sha256Base64("/no/such/file"), std::runtime_error);
    }
};

QTEST_APPLESS_MAIN(HashTest)
#include "HashTest.moc"
