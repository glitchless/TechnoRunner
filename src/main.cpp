#include <QApplication>
#include <QThread>
#include <QtGlobal>
#include <QFile>
#include <QDateTime>
#include <QMutex>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <exception>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif
#include "ui/SplashScreen.h"
#include "app/Bootstrapper.h"
#include "run/Launcher.h"
#include "util/Paths.h"

using namespace tprunner;

// ---------------------------------------------------------------------------
// Crash/exit instrumentation.
//
// Goal: log EVERY reason the app can close, so a "crashed on launch" report
// tells us which path fired — clean exit, user close, unhandled C++ exception,
// fatal Qt message, or a hard crash signal (segfault/abort).
//
// All output is mirrored to stdout (visible from a terminal) AND appended to a
// raw fd on <base>/runner.log (the only output you can see when the app is
// launched as a .app bundle from Finder). The raw fd lets the signal handler
// write without touching async-signal-unsafe Qt/stdio machinery.
// ---------------------------------------------------------------------------

static int g_logFd = -1;  // raw fd into runner.log for signal-safe writes

// Append to the runner.log file only (used by the normal, stdout-bound logger).
static void writeLogFile(const char* s) {
#ifdef Q_OS_UNIX
    if (g_logFd >= 0) { ssize_t r = ::write(g_logFd, s, std::strlen(s)); (void)r; }
#endif
}

// Write to stderr AND runner.log. Used by crash/fatal paths where stdio buffering
// is unsafe; stderr is the terminal-visible channel for crashes.
static void writeCrash(const char* s) {
#ifdef Q_OS_UNIX
    ssize_t r = ::write(STDERR_FILENO, s, std::strlen(s)); (void)r;
#else
    std::fputs(s, stderr);
    std::fflush(stderr);
#endif
    writeLogFile(s);
}

// Qt message handler: timestamped, leveled, mirrored to stdout + runner.log.
static void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg) {
    static QMutex mutex;
    QMutexLocker lock(&mutex);

    const char* level = "INFO";
    switch (type) {
        case QtDebugMsg:    level = "DEBUG"; break;
        case QtInfoMsg:     level = "INFO";  break;
        case QtWarningMsg:  level = "WARN";  break;
        case QtCriticalMsg: level = "CRIT";  break;
        case QtFatalMsg:    level = "FATAL"; break;
    }
    const QString line = QStringLiteral("[%1] %2: %3")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
             QString::fromLatin1(level), msg);

    std::fprintf(stdout, "%s\n", line.toLocal8Bit().constData());
    std::fflush(stdout);
    const QByteArray b = line.toUtf8() + '\n';
    writeLogFile(b.constData());

    if (type == QtFatalMsg) {
        writeCrash("[FATAL] app closing: qFatal()\n");
        std::abort();
    }
}

// Hard crash signals: log which one, then restore the default handler and
// re-raise so we still get the real crash behaviour / core dump.
extern "C" void crashSignalHandler(int sig) {
    const char* name;
    switch (sig) {
        case SIGSEGV: name = "[FATAL] app closing: SIGSEGV (segmentation fault)\n"; break;
        case SIGABRT: name = "[FATAL] app closing: SIGABRT (abort)\n"; break;
        case SIGFPE:  name = "[FATAL] app closing: SIGFPE (arithmetic error)\n"; break;
        case SIGILL:  name = "[FATAL] app closing: SIGILL (illegal instruction)\n"; break;
#ifdef SIGBUS
        case SIGBUS:  name = "[FATAL] app closing: SIGBUS (bus error)\n"; break;
#endif
        default:      name = "[FATAL] app closing: crash signal\n"; break;
    }
    writeCrash(name);
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

// Unhandled C++ exception escaping to std::terminate.
static std::terminate_handler g_prevTerminate = nullptr;
static void terminateHandler() {
    QString what = QStringLiteral("(non-std exception)");
    if (std::exception_ptr e = std::current_exception()) {
        try { std::rethrow_exception(e); }
        catch (const std::exception& ex) { what = QString::fromLocal8Bit(ex.what()); }
        catch (...) {}
    } else {
        what = QStringLiteral("(no active exception)");
    }
    qCritical().noquote() << "app closing: std::terminate — unhandled exception:" << what;
    if (g_prevTerminate) g_prevTerminate();
    std::abort();
}

static void installCrashHandlers() {
    const QByteArray path = Paths::runnerLog().toLocal8Bit();
    std::FILE* f = std::fopen(path.constData(), "w");  // truncate at startup
    if (f) {
#ifdef Q_OS_UNIX
        g_logFd = ::fileno(f);
#endif
    }

    qInstallMessageHandler(messageHandler);
    g_prevTerminate = std::set_terminate(terminateHandler);

    std::signal(SIGSEGV, crashSignalHandler);
    std::signal(SIGABRT, crashSignalHandler);
    std::signal(SIGFPE,  crashSignalHandler);
    std::signal(SIGILL,  crashSignalHandler);
#ifdef SIGBUS
    std::signal(SIGBUS,  crashSignalHandler);
#endif
}

int main(int argc, char** argv) {
    installCrashHandlers();
    qInfo() << "=== TechnoRunner starting === log file:" << Paths::runnerLog();

    QApplication app(argc, argv);

    // Event-loop-driven quit (e.g. last window closed, QApplication::quit()).
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [] {
        qInfo() << "app closing: QCoreApplication::aboutToQuit (event loop quit)";
    });

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
        // Read the manifest-resolved java path before deleting boot below.
        const bool launched = Launcher::run(boot->resolvedJavaPath(), Paths::launcherFile());
        qInfo() << "game process launch requested; startDetached returned" << launched;
        thread->quit();
        thread->wait();
        delete boot;           // worker thread has stopped; safe to delete here
        qInfo() << "app closing: normal shutdown after launch, exit(0)";
        app.exit(0);
    });

    thread->start();
    const int rc = app.exec();
    qInfo() << "app closing: event loop returned" << rc;
    return rc;
}
