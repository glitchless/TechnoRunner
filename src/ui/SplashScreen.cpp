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
    connect(closeBtn, &QPushButton::clicked, this, []{ std::exit(0); });

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

}
