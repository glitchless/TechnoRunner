#pragma once
#include <QString>

namespace tprunner {

class ProgressMonitor {
public:
    virtual ~ProgressMonitor() = default;
    virtual void setProgress(int progress) = 0;
    virtual void setMax(int max) = 0;
    virtual void incrementProgress(int amount) = 0;
    virtual void setStatus(const QString& status) = 0;
};

}
