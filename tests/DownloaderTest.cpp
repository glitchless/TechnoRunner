#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QNetworkAccessManager>
#include <QFile>
#include "net/Downloader.h"
#include "app/ProgressMonitor.h"

using namespace tprunner;

namespace {
class RecordingMonitor : public ProgressMonitor {
public:
    int lastProgress = -1, lastMax = -1; QString lastStatus;
    void setProgress(int p) override { lastProgress = p; }
    void setMax(int m) override { lastMax = m; }
    void incrementProgress(int a) override { lastProgress += a; }
    void setStatus(const QString& s) override { lastStatus = s; }
};

// Minimal one-shot HTTP/1.1 server returning a fixed body.
class StubServer : public QObject {
    Q_OBJECT
public:
    QByteArray body;
    quint16 port() const { return server_.serverPort(); }
    bool start() {
        if (!server_.listen(QHostAddress::LocalHost)) return false;
        connect(&server_, &QTcpServer::newConnection, this, [this] {
            QTcpSocket* s = server_.nextPendingConnection();
            connect(s, &QTcpSocket::readyRead, this, [this, s] {
                s->readAll();
                QByteArray resp = "HTTP/1.1 200 OK\r\nContent-Length: "
                    + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
                s->write(resp); s->flush(); s->disconnectFromHost();
            });
        });
        return true;
    }
private:
    QTcpServer server_;
};
}

class DownloaderTest : public QObject {
    Q_OBJECT
private slots:
    void httpGetReturnsBody() {
        StubServer srv; srv.body = "hello-body"; QVERIFY(srv.start());
        QNetworkAccessManager nam; Downloader dl(&nam);
        const QByteArray out = dl.httpGet(QString("http://127.0.0.1:%1/x").arg(srv.port()));
        QCOMPARE(out, QByteArray("hello-body"));
    }
    void downloadToFileWritesAndReportsProgress() {
        StubServer srv; srv.body = QByteArray(2048, 'z'); QVERIFY(srv.start());
        QNetworkAccessManager nam; Downloader dl(&nam);
        QTemporaryDir dir; const QString dest = dir.path() + "/out.bin";
        RecordingMonitor mon;
        dl.downloadToFile(QString("http://127.0.0.1:%1/x").arg(srv.port()), dest, &mon);
        QFile f(dest); QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll().size(), 2048);
        QCOMPARE(mon.lastMax, 2048);
        QVERIFY(mon.lastProgress > 0);
    }
    void httpGetOnConnectionRefusedThrows() {
        QNetworkAccessManager nam; Downloader dl(&nam);
        QVERIFY_EXCEPTION_THROWN(dl.httpGet("http://127.0.0.1:1/nope"), std::runtime_error);
    }
};

QTEST_MAIN(DownloaderTest)
#include "DownloaderTest.moc"
