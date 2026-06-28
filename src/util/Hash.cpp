#include "util/Hash.h"
#include <QFile>
#include <QCryptographicHash>
#include <stdexcept>

namespace tprunner {

QString sha256Base64(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error(("cannot open file: " + filePath).toStdString());

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
        throw std::runtime_error(("cannot read file: " + filePath).toStdString());

    return QString::fromLatin1(hash.result().toBase64());
}

}
