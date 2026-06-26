#include "ui/GProgressBar.h"

namespace tprunner {

GProgressBar::GProgressBar(QWidget* parent) : QProgressBar(parent) {
    setTextVisible(false);
    setMinimumHeight(20);
    setStyleSheet(
        "QProgressBar {"
        "  border: none;"
        "  border-radius: 4px;"
        "  background: #ddddde;"
        "}"
        "QProgressBar::chunk {"
        "  border-radius: 4px;"
        "  background: #00db9d;"
        "}");
}

}
