#pragma once
#include <QWidget>

class QLabel;

namespace tprunner {

class GProgressBar;

class SplashScreen : public QWidget {
    Q_OBJECT
public:
    explicit SplashScreen(QWidget* parent = nullptr);

    QString statusText() const;
    int progressValue() const;

public slots:
    void onStatus(const QString& status);
    void onProgress(int progress);
    void onMax(int max);

private:
    QLabel* background_ = nullptr;
    QLabel* label_ = nullptr;
    GProgressBar* bar_ = nullptr;
};

}
