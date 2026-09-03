#include "FakePlatform.h"

namespace eink::tests {

FakePlatform::FakePlatform(bool available,bool lightAvailable,bool directNightAvailableValue):saturationAvailable(available),lightModeAvailable(lightAvailable),nightAvailable(lightAvailable),directNightAvailable(directNightAvailableValue&&available) {
    DisplayInfo external;external.stableId=QStringLiteral("fake-external");external.deviceName=QStringLiteral("FAKE1");external.friendlyName=QStringLiteral("Bigme B251 Pro");external.colorAdjustmentSupported=available;external.acmSupported=available;external.acmEnabled=available;external.ditheringControlSupported=true;displayList.push_back(external);
    DisplayInfo builtIn;builtIn.stableId=QStringLiteral("fake-built-in");builtIn.deviceName=QStringLiteral("FAKE2");builtIn.friendlyName=QStringLiteral("Built-in Display");builtIn.builtIn=true;builtIn.colorAdjustmentSupported=available;builtIn.acmSupported=available;displayList.push_back(builtIn);
}

ApplyResult FakePlatform::applyToneCurve(const DisplayInfo &d,const ToneCurve &c){curves[d.stableId]=c;++curveApplyCalls;operationLog<<QStringLiteral("curve.apply");return ApplyResult::ok();}
ApplyResult FakePlatform::restoreToneCurve(const DisplayInfo &d){curves.remove(d.stableId);++curveRestoreCalls;operationLog<<QStringLiteral("curve.restore");return ApplyResult::ok();}
ApplyResult FakePlatform::applyColor(const DisplayInfo &d,double s,const RgbBalance &b){saturations[d.stableId]=s;balances[d.stableId]=b;++colorApplyCalls;operationLog<<QStringLiteral("color.apply");return ApplyResult::ok();}
ApplyResult FakePlatform::restoreColor(const DisplayInfo &d){saturations.remove(d.stableId);balances.remove(d.stableId);++colorRestoreCalls;operationLog<<QStringLiteral("color.restore");return ApplyResult::ok();}
ApplyResult FakePlatform::beginColorSafetyTest(const DisplayInfo &d,double s,const RgbBalance &b,int){++colorSafetyBeginCalls;operationLog<<QStringLiteral("color.safety.begin");if(!colorSafetyBeginResult.success)return colorSafetyBeginResult;return applyColor(d,s,b);}
ApplyResult FakePlatform::confirmColorSafetyTest(const DisplayInfo &){++colorSafetyConfirmCalls;operationLog<<QStringLiteral("color.safety.confirm");return ApplyResult::ok();}
ApplyResult FakePlatform::rollbackColorSafetyTest(const DisplayInfo &d){++colorSafetyRollbackCalls;operationLog<<QStringLiteral("color.safety.rollback");return restoreColor(d);}
ApplyResult FakePlatform::setDitheringDisabled(const DisplayInfo &,bool){++ditherCalls;return ApplyResult::ok();}

} // namespace eink::tests
