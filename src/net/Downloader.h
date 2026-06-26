#pragma once
#include <QString>
#include <QByteArray>

class QNetworkAccessManager;

namespace tprunner {

class ProgressMonitor;

class Downloader {
public:
    explicit Downloader(QNetworkAccessManager* nam);
    QByteArray httpGet(const QString& url);
    void downloadToFile(const QString& url, const QString& destPath, ProgressMonitor* monitor);
private:
    QNetworkAccessManager* nam_;
};

}
