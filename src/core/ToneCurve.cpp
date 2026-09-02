#include "ToneCurve.h"

#include <algorithm>
#include <cmath>

namespace eink {

ToneCurve ToneCurve::identity() { return {}; }

double ToneCurve::value(double input) const {
    double v = std::max(0.0, std::min(input, 1.0));
    const double lo = blackPoint;
    const double hi = std::max(whitePoint, blackPoint + 0.001);
    if (lo > 0.0 || hi < 1.0)
        v = std::max(0.0, std::min((v - lo) / (hi - lo), 1.0));
    if (knee <= 0.0 || v <= 0.0 || v >= knee)
        return v;
    const double u = std::max(0.0, std::min(v / knee, 1.0));
    const double weight = 1.0 - (u * u * (3.0 - 2.0 * u));
    return std::max(0.0, std::min((1.0 - weight) * v + weight * std::pow(v, gamma), 1.0));
}

QVector<quint16> ToneCurve::table(int count) const {
    QVector<quint16> result;
    if (count < 2)
        return result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        const double x = static_cast<double>(i) / static_cast<double>(count - 1);
        result.push_back(static_cast<quint16>(std::lround(value(x) * 65535.0)));
    }
    return result;
}

bool ToneCurve::isIdentity() const {
    return (std::abs(gamma - 1.0) < 0.001 || knee <= 0.0)
        && blackPoint <= 0.0 && whitePoint >= 1.0;
}

bool ToneCurve::isMonotonic(int samples) const {
    double previous = -1.0;
    for (int i = 0; i < samples; ++i) {
        const double current = value(static_cast<double>(i) / static_cast<double>(samples - 1));
        if (current < previous - 1e-9)
            return false;
        previous = current;
    }
    return true;
}

bool ToneCurve::operator==(const ToneCurve &o) const {
    return std::abs(knee - o.knee) < 1e-9 && std::abs(gamma - o.gamma) < 1e-9
        && std::abs(blackPoint - o.blackPoint) < 1e-9 && std::abs(whitePoint - o.whitePoint) < 1e-9;
}

ToneCurve curveForTextLevel(TextLevel level) {
    switch (level) {
    case TextLevel::Medium: return {0.65, 2.10, 0.0, 1.0};
    case TextLevel::Strong: return {0.80, 2.70, 0.0, 1.0};
    case TextLevel::Sharp: return {1.00, 5.00, 0.10, 1.0};
    case TextLevel::Solid: return {1.00, 6.00, 0.34, 1.0};
    case TextLevel::Off: return ToneCurve::identity();
    }
    return ToneCurve::identity();
}

ToneCurve curveForEnhanceLevel(EnhanceLevel level) {
    switch (level) {
    case EnhanceLevel::Subtle: return {0.25, 0.75, 0.0, 1.0};
    case EnhanceLevel::Medium: return {0.35, 0.60, 0.0, 1.0};
    case EnhanceLevel::Strong: return {0.45, 0.45, 0.0, 1.0};
    case EnhanceLevel::Off: return ToneCurve::identity();
    }
    return ToneCurve::identity();
}

} // namespace eink
