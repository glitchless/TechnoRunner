#pragma once
#include <QString>
#include <QStringList>

namespace tprunner {

enum class Os;

struct LaunchCommand { QString program; QStringList arguments; };

class Launcher {
public:
    static LaunchCommand buildCommand(const QString& javaPath, const QString& jarPath,
                                      const QString& outLog, const QString& errLog, Os os);
    static bool run();
};

}
