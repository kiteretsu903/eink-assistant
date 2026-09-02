#include "AppState.h"

#include <cmath>

namespace eink {

bool RgbBalance::isIdentity() const {
    return std::abs(red - 1.0) < 0.001 && std::abs(green - 1.0) < 0.001
        && std::abs(blue - 1.0) < 0.001;
}

ToneCurve DisplaySettings::effectiveCurve() const {
    if (!isEink)
        return ToneCurve::identity();
    if (advanced)
        return customCurve;
    if (textLevel != TextLevel::Off)
        return curveForTextLevel(textLevel);
    return curveForEnhanceLevel(enhanceLevel);
}

DisplaySettings &AppSettings::forDisplay(const QString &stableId) {
    for (DisplaySettings &entry : displays) {
        if (entry.stableId == stableId)
            return entry;
    }
    displays.push_back(DisplaySettings{});
    displays.back().stableId = stableId;
    return displays.back();
}

} // namespace eink
