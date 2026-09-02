#pragma once

#include <QVector>

namespace eink {

struct ToneCurve {
    double knee = 0.35;
    double gamma = 1.0;
    double blackPoint = 0.0;
    double whitePoint = 1.0;

    static ToneCurve identity();
    double value(double input) const;
    QVector<quint16> table(int count = 256) const;
    bool isIdentity() const;
    bool isMonotonic(int samples = 1024) const;
    bool operator==(const ToneCurve &other) const;
};

enum class TextLevel { Off, Medium, Strong, Sharp, Solid };
enum class EnhanceLevel { Off, Subtle, Medium, Strong };

ToneCurve curveForTextLevel(TextLevel level);
ToneCurve curveForEnhanceLevel(EnhanceLevel level);

} // namespace eink
