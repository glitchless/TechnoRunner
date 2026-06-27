#include "ui/SplashScreen.h"
#include "ui/GProgressBar.h"
#include <QLabel>
#include <QPixmap>
#include <QIcon>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFontDatabase>
#include <QFont>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>
#include <QMouseEvent>
#include <QDebug>
#include <cstdlib>

namespace tprunner {

namespace {
QString pickFont() {
    const QStringList families = QFontDatabase::families();
    for (const QString& wanted : {QStringLiteral("Helvetica"), QStringLiteral("Arial")})
        if (families.contains(wanted)) return wanted;
    return QString();
}
}

SplashScreen::SplashScreen(QWidget* parent) : QWidget(parent) {
    setWindowFlag(Qt::FramelessWindowHint);
    setWindowTitle(QStringLiteral("Загрузка..."));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Background with overlaid close button.
    background_ = new QLabel(this);
    const QPixmap bg(QStringLiteral(":/background.jpeg"));
    background_->setPixmap(bg);
    background_->setFixedSize(bg.size());

    auto* closeBtn = new QPushButton(background_);
    const QPixmap closeIcon(QStringLiteral(":/close-btn.png"));
    closeBtn->setIcon(QIcon(closeIcon));
    closeBtn->setIconSize(closeIcon.size());
    closeBtn->setFixedSize(closeIcon.size());
    closeBtn->setFlat(true);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("border: none; background: transparent;");
    closeBtn->move(bg.width() - closeIcon.width() - 13, 13);
    connect(closeBtn, &QPushButton::clicked, this, []{
        qInfo() << "app closing: splash close button clicked, std::exit(0)";
        std::exit(0);
    });

    root->addWidget(background_);

    // Status panel.
    auto* statusPanel = new QWidget(this);
    statusPanel->setStyleSheet("background: #303135;");
    auto* statusLayout = new QVBoxLayout(statusPanel);
    statusLayout->setContentsMargins(20, 15, 20, 15);
    statusLayout->setSpacing(8);

    label_ = new QLabel(QStringLiteral("Загрузка..."), statusPanel);
    label_->setAlignment(Qt::AlignCenter);
    QFont font(pickFont()); font.setPointSize(14); label_->setFont(font);
    label_->setStyleSheet("color: #ddddde;");
    statusLayout->addWidget(label_);

    bar_ = new GProgressBar(statusPanel);
    statusLayout->addWidget(bar_);

    root->addWidget(statusPanel);

    setFixedSize(sizeHint());
    if (auto* scr = QGuiApplication::primaryScreen())
        move(scr->geometry().center() - rect().center());
}

QString SplashScreen::statusText() const { return label_->text(); }
int SplashScreen::progressValue() const { return bar_->value(); }

void SplashScreen::onStatus(const QString& status) { label_->setText(status); }
void SplashScreen::onProgress(int progress) { bar_->setValue(progress); }
void SplashScreen::onMax(int max) { bar_->setMaximum(max); }

// Drag-to-move: record the cursor's offset from the window origin on press, then keep
// the window at (cursor - offset) while dragging. Child widgets that ignore mouse events
// (the background QLabel, the status panel) propagate the press up to here; the close
// QPushButton consumes its own clicks, so it stays clickable.
void SplashScreen::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        dragging_ = true;
        dragOffset_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    }
}
void SplashScreen::mouseMoveEvent(QMouseEvent* e) {
    if (dragging_ && (e->buttons() & Qt::LeftButton)) {
        move(e->globalPosition().toPoint() - dragOffset_);
        e->accept();
    }
}
void SplashScreen::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) { dragging_ = false; e->accept(); }
}

}
