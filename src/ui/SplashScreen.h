#pragma once
#include <QWidget>
#include <QPoint>

class QLabel;
class QMouseEvent;

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

protected:
    // Frameless windows have no titlebar, so drag-to-move is implemented by hand.
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    QLabel* background_ = nullptr;
    QLabel* label_ = nullptr;
    GProgressBar* bar_ = nullptr;
    QPoint dragOffset_;
    bool dragging_ = false;
};

}
