#include "net/Downloader.h"
#include "app/ProgressMonitor.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QUrl>
#include <stdexcept>

namespace tprunner {

Downloader::Downloader(QNetworkAccessManager* nam) : nam_(nam) {}

namespace {
QNetworkRequest makeRequest(const QString& url) {
    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    return req;
}
}

QByteArray Downloader::httpGet(const QString& url) {
    QNetworkReply* reply = nam_->get(makeRequest(url));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const QNetworkReply::NetworkError err = reply->error();
    const QByteArray data = reply->readAll();
    reply->deleteLater();
    if (err != QNetworkReply::NoError)
        throw std::runtime_error(("GET failed: " + url).toStdString());
    return data;
}

void Downloader::downloadToFile(const QString& url, const QString& destPath, ProgressMonitor* monitor) {
    QFile file(destPath);
    if (!file.open(QIODevice::WriteOnly))
        throw std::runtime_error(("cannot open for write: " + destPath).toStdString());

    QNetworkReply* reply = nam_->get(makeRequest(url));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::readyRead, reply, [&] {
        file.write(reply->readAll());
    });
    if (monitor) {
        QObject::connect(reply, &QNetworkReply::downloadProgress, reply,
            [monitor](qint64 received, qint64 total) {
                if (total > 0) { monitor->setMax(int(total)); monitor->setProgress(int(received)); }
            });
    }
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    file.write(reply->readAll());
    file.flush();
    file.close();
    const QNetworkReply::NetworkError err = reply->error();
    reply->deleteLater();
    if (err != QNetworkReply::NoError) {
        QFile::remove(destPath);
        throw std::runtime_error(("download failed: " + url).toStdString());
    }
}

}
