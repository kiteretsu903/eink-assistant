#pragma once

#include <QByteArray>
#include <QString>

namespace eink {

struct NightLightStateRecord {
    bool enabled = false;
    qint32 initialized = 0;
    quint64 cloudTimestamp = 0;
    quint64 transitionFileTime = 0;
};

// Codec for Windows.Data.BlueLightReductionState. The binary layout is a
// CloudStore envelope containing a Microsoft Bond CompactBinary v1 record.
class NightLightStateCodec {
public:
    static bool decode(const QByteArray &data, NightLightStateRecord *record, QString *error = nullptr);
    static QByteArray encode(const NightLightStateRecord &record);
};

} // namespace eink
