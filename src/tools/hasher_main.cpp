#include "util/Hash.h"
#include <QCoreApplication>
#include <cstdio>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { std::fprintf(stderr, "usage: technorunner-hasher <file>\n"); return 2; }
    std::printf("%s\n", tprunner::sha256Base64(QString::fromLocal8Bit(argv[1])).toUtf8().constData());
    return 0;
}
