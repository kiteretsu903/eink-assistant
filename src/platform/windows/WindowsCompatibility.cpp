#include "WindowsCompatibility.h"

namespace eink::windows {

ColorPipeline chooseColorPipeline(quint32 build,
                                  bool modernProfileApisAvailable,
                                  bool mhc2CapabilityApiAvailable,
                                  bool mhc2Supported,
                                  bool acmSupported) {
    if (build >= 26100)
        return modernProfileApisAvailable && acmSupported
            ? ColorPipeline::Windows11Acm : ColorPipeline::Unavailable;

    if (build < 19041 || build >= 22000 || !modernProfileApisAvailable)
        return ColorPipeline::Unavailable;

    if (mhc2CapabilityApiAvailable)
        return mhc2Supported ? ColorPipeline::Windows10Mhc2 : ColorPipeline::Unavailable;

    return ColorPipeline::Windows10Mhc2;
}

} // namespace eink::windows
