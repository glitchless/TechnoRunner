#include <QApplication>
#include <QWidget>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QWidget w;
    w.resize(400, 200);
    w.show();
    return app.exec();
}
