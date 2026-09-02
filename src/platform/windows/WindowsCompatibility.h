#pragma once

#include <QtGlobal>

namespace eink::windows {

enum class ColorPipeline {
    Unavailable,
    Windows10Mhc2,
    Windows11Acm
};

ColorPipeline chooseColorPipeline(quint32 build,
                                  bool modernProfileApisAvailable,
                                  bool mhc2CapabilityApiAvailable,
                                  bool mhc2Supported,
                                  bool acmSupported);

} // namespace eink::windows
