#pragma once
#include <QObject>
#include <QString>
#include <functional>
#include "app/ProgressMonitor.h"

namespace tprunner {

// Retry `block` up to `attemptsNumber` times. On failure of attempt N (0-based), wait
// 2^N seconds, showing a per-second countdown via `monitor.setStatus`, sleeping through
// `sleepSeconds(1)` each tick. Returns true on first success, false if all attempts fail.
bool tryExponential(int attemptsNumber, ProgressMonitor& monitor,
                    const std::function<void()>& block,
                    const std::function<void(int seconds)>& sleepSeconds);

class Bootstrapper : public QObject, public ProgressMonitor {
    Q_OBJECT
public:
    explicit Bootstrapper(QObject* parent = nullptr);

    void setProgress(int progress) override;
    void setMax(int max) override;
    void incrementProgress(int amount) override;
    void setStatus(const QString& status) override;

public slots:
    void run();

signals:
    void progressChanged(int progress);
    void maxChanged(int max);
    void statusChanged(const QString& status);
    void finished();

private:
    void checkAndDownloadAll();
    int current_ = 0;
};

}
