#include "WindowsCompatibility.h"

namespace eink::windows {

ColorPipeline chooseColorPipeline(quint32 build,
                                  bool modernProfileApisAvailable,
                                  bool mhc2CapabilityApiAvailable,
                                  bool mhc2Supported,
                                  bool acmSupported,
                                  bool matrixDdiSupported,
                                  bool wddm26OrLater,
                                  bool exactTargetMapped) {
    if (build >= 26100)
        return modernProfileApisAvailable && acmSupported
            ? ColorPipeline::Windows11Acm : ColorPipeline::Unavailable;

    if (build < 19041 || build >= 22000 || !modernProfileApisAvailable
        || !matrixDdiSupported || !wddm26OrLater || !exactTargetMapped)
        return ColorPipeline::Unavailable;

    if (mhc2CapabilityApiAvailable)
        return mhc2Supported ? ColorPipeline::Windows10Mhc2 : ColorPipeline::Unavailable;

    return ColorPipeline::Windows10Mhc2;
}

NightLightControlPath chooseNightLightControlPath(quint32 build) {
    if (build >= 22000) return NightLightControlPath::Windows11UiAutomation;
    if (build >= 19041) return NightLightControlPath::Windows10Registry;
    return NightLightControlPath::Unavailable;
}

} // namespace eink::windows
