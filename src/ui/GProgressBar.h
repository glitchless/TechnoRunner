#pragma once
#include <QProgressBar>

namespace tprunner {

class GProgressBar : public QProgressBar {
    Q_OBJECT
public:
    explicit GProgressBar(QWidget* parent = nullptr);
};

}
