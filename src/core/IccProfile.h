#pragma once

#include "AppState.h"

#include <QByteArray>
#include <QString>

namespace eink {

struct IccBaseProfile {
    double matrix[9] = {
        0.4360747, 0.3850649, 0.1430804,
        0.2225045, 0.7168786, 0.0606169,
        0.0139322, 0.0971045, 0.7141733
    };
    double whitePoint[3] = {0.9642, 1.0, 0.8249};
    double gamma = 2.2;
};

class IccProfile {
public:
    static QByteArray make(double saturation, const RgbBalance &balance,
                           const QString &description,
                           const IccBaseProfile &base = IccBaseProfile{});
    static bool parseBase(const QByteArray &data, IccBaseProfile *base);
    static QString description(const QByteArray &data);
    static bool structurallyValid(const QByteArray &data, QString *error = nullptr);
    static double matrixDeterminant(const QByteArray &data);
    static bool mhc2Matrix(const QByteArray &data, double matrix[9]);
};

} // namespace eink
