#pragma once

#include <QtGlobal>

namespace eink::windows {

enum class ColorPipeline {
    Unavailable,
    Windows10Mhc2,
    Windows11Acm
};

enum class NightLightControlPath {
    Unavailable,
    Windows10Registry,
    Windows11UiAutomation
};

ColorPipeline chooseColorPipeline(quint32 build,
                                  bool modernProfileApisAvailable,
                                  bool mhc2CapabilityApiAvailable,
                                  bool mhc2Supported,
                                  bool acmSupported,
                                  bool matrixDdiSupported,
                                  bool wddm26OrLater,
                                  bool exactTargetMapped);

NightLightControlPath chooseNightLightControlPath(quint32 build);

} // namespace eink::windows
