#include <QApplication>
#include <QThread>
#include "ui/SplashScreen.h"
#include "app/Bootstrapper.h"
#include "run/Launcher.h"

using namespace tprunner;

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    SplashScreen splash;
    splash.show();

    auto* thread = new QThread(&app);
    auto* boot = new Bootstrapper;
    boot->moveToThread(thread);

    QObject::connect(boot, &Bootstrapper::statusChanged,   &splash, &SplashScreen::onStatus);
    QObject::connect(boot, &Bootstrapper::progressChanged, &splash, &SplashScreen::onProgress);
    QObject::connect(boot, &Bootstrapper::maxChanged,      &splash, &SplashScreen::onMax);
    QObject::connect(thread, &QThread::started, boot, &Bootstrapper::run);

    QObject::connect(boot, &Bootstrapper::finished, &app, [&] {
        splash.hide();
        Launcher::run();
        thread->quit();
        thread->wait();
        delete boot;           // worker thread has stopped; safe to delete here
        app.exit(0);
    });

    thread->start();
    return app.exec();
}
