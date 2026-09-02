#include "NightLightStateCodec.h"

#include <limits>

namespace eink {
namespace {

// The format mapping is derived from win-nightlight-cli by Kevin Xiao (MIT).
// See THIRD-PARTY-NOTICES-WINDOWS.md for attribution and license text.
enum BondType : quint8 {
    Stop = 0, StopBase = 1, Bool = 2, UInt64 = 6, Struct = 10,
    List = 11, Int8 = 14, Int32 = 16
};

struct Field { quint8 type = Stop; quint16 id = 0; };

class Reader {
public:
    explicit Reader(const QByteArray &data) : m_data(data) {}

    bool header() {
        static const QByteArray magic = QByteArray::fromHex("43420100");
        if (m_data.mid(m_pos, magic.size()) != magic) return fail(QStringLiteral("invalid CompactBinary header"));
        m_pos += magic.size(); return true;
    }
    bool field(Field *field) {
        quint8 tag = 0; if (!byte(&tag)) return false;
        field->type = tag & 0x1f;
        const quint8 idBits = tag & 0xe0;
        if (idBits == 0xc0) {
            quint8 id = 0; if (!byte(&id)) return false; field->id = id;
        } else if (idBits == 0xe0) {
            quint8 low = 0, high = 0; if (!byte(&low) || !byte(&high)) return false;
            field->id = static_cast<quint16>(low | (static_cast<quint16>(high) << 8));
        } else {
            field->id = static_cast<quint16>(tag >> 5);
        }
        return true;
    }
    bool expect(quint8 type, quint16 id) {
        Field value; if (!field(&value)) return false;
        if (value.type != type || value.id != id)
            return fail(QStringLiteral("unexpected field %1/%2").arg(value.id).arg(value.type));
        return true;
    }
    bool stop() { return expect(Stop, 0); }
    bool byte(quint8 *value) {
        if (m_pos >= m_data.size()) return fail(QStringLiteral("truncated payload"));
        *value = static_cast<quint8>(m_data.at(m_pos++)); return true;
    }
    bool varUInt(quint64 *value) {
        quint64 result = 0;
        for (int shift = 0; shift < 64; shift += 7) {
            quint8 current = 0; if (!byte(&current)) return false;
            if (shift == 63 && (current & 0xfe)) return fail(QStringLiteral("varint overflow"));
            result |= static_cast<quint64>(current & 0x7f) << shift;
            if (!(current & 0x80)) { *value = result; return true; }
        }
        return fail(QStringLiteral("varint overflow"));
    }
    bool int32(qint32 *value) {
        quint64 encoded = 0; if (!varUInt(&encoded) || encoded > std::numeric_limits<quint32>::max())
            return fail(QStringLiteral("int32 overflow"));
        *value = static_cast<qint32>((encoded >> 1) ^ static_cast<quint64>(-static_cast<qint64>(encoded & 1)));
        return true;
    }
    bool raw(int count, QByteArray *value) {
        if (count < 0 || count > m_data.size() - m_pos) return fail(QStringLiteral("invalid list length"));
        *value = m_data.mid(m_pos, count); m_pos += count; return true;
    }
    bool atEnd() const { return m_pos == m_data.size(); }
    QString error() const { return m_error; }

private:
    bool fail(const QString &message) { if (m_error.isEmpty()) m_error = message; return false; }
    const QByteArray &m_data;
    int m_pos = 0;
    QString m_error;
};

void writeVarUInt(QByteArray *data, quint64 value) {
    do {
        quint8 byte = static_cast<quint8>(value & 0x7f); value >>= 7;
        if (value) byte |= 0x80; data->append(static_cast<char>(byte));
    } while (value);
}

void writeField(QByteArray *data, quint8 type, quint16 id) {
    if (id <= 5) data->append(static_cast<char>(type | (id << 5)));
    else if (id <= 255) { data->append(static_cast<char>(type | 0xc0)); data->append(static_cast<char>(id)); }
    else { data->append(static_cast<char>(type | 0xe0)); data->append(static_cast<char>(id & 0xff)); data->append(static_cast<char>(id >> 8)); }
}

void writeInt32(QByteArray *data, qint32 value) {
    const quint32 encoded = (static_cast<quint32>(value) << 1) ^ static_cast<quint32>(value >> 31);
    writeVarUInt(data, encoded);
}

bool decodeInner(const QByteArray &inner, NightLightStateRecord *record, QString *error) {
    Reader reader(inner); if (!reader.header()) { if (error) *error = reader.error(); return false; }
    bool initializedSeen = false, transitionSeen = false, enabledSeen = false;
    for (;;) {
        Field field; if (!reader.field(&field)) break;
        if (field.type == Stop && field.id == 0) {
            if (!reader.atEnd()) { if (error) *error = QStringLiteral("trailing inner-state bytes"); return false; }
            if (!initializedSeen || !transitionSeen) { if (error) *error = QStringLiteral("required Night Light state fields are missing"); return false; }
            record->enabled = enabledSeen; return true;
        }
        if (field.id == 0 && field.type == Int32 && !enabledSeen) {
            qint32 value = 0; if (!reader.int32(&value) || value != 0) break; enabledSeen = true;
        } else if (field.id == 10 && field.type == Int32 && !initializedSeen) {
            if (!reader.int32(&record->initialized) || record->initialized != 1) break; initializedSeen = true;
        } else if (field.id == 20 && field.type == UInt64 && !transitionSeen) {
            if (!reader.varUInt(&record->transitionFileTime) || !record->transitionFileTime) break; transitionSeen = true;
        } else {
            if (error) *error = QStringLiteral("unsupported Night Light state field %1/%2").arg(field.id).arg(field.type);
            return false;
        }
    }
    if (error) *error = reader.error().isEmpty() ? QStringLiteral("malformed Night Light state") : reader.error();
    return false;
}

} // namespace

bool NightLightStateCodec::decode(const QByteArray &data, NightLightStateRecord *record, QString *error) {
    if (!record) { if (error) *error = QStringLiteral("missing output record"); return false; }
    NightLightStateRecord decoded;
    Reader reader(data);
    quint8 metadata = 0, elementType = 0; quint64 count = 0; QByteArray inner;
    const bool valid = reader.header()
        && reader.expect(Struct, 0) && reader.expect(Bool, 0) && reader.byte(&metadata) && metadata == 1 && reader.stop()
        && reader.expect(Struct, 1) && reader.expect(UInt64, 0) && reader.varUInt(&decoded.cloudTimestamp)
        && reader.expect(Struct, 1) && reader.expect(List, 1) && reader.byte(&elementType) && elementType == Int8
        && reader.varUInt(&count) && count <= 4096 && reader.raw(static_cast<int>(count), &inner)
        && reader.stop() && reader.stop() && reader.stop() && reader.atEnd();
    if (!valid) {
        if (error) *error = reader.error().isEmpty() ? QStringLiteral("unsupported CloudStore envelope") : reader.error();
        return false;
    }
    if (!decoded.cloudTimestamp) { if (error) *error = QStringLiteral("invalid CloudStore timestamp"); return false; }
    if (!decodeInner(inner, &decoded, error)) return false;
    *record = decoded; return true;
}

QByteArray NightLightStateCodec::encode(const NightLightStateRecord &record) {
    QByteArray inner = QByteArray::fromHex("43420100");
    if (record.enabled) { writeField(&inner, Int32, 0); writeInt32(&inner, 0); }
    writeField(&inner, Int32, 10); writeInt32(&inner, 1);
    writeField(&inner, UInt64, 20); writeVarUInt(&inner, record.transitionFileTime);
    writeField(&inner, Stop, 0);

    QByteArray result = QByteArray::fromHex("43420100");
    writeField(&result, Struct, 0); writeField(&result, Bool, 0); result.append(char(1)); writeField(&result, Stop, 0);
    writeField(&result, Struct, 1); writeField(&result, UInt64, 0); writeVarUInt(&result, record.cloudTimestamp);
    writeField(&result, Struct, 1); writeField(&result, List, 1); result.append(static_cast<char>(Int8)); writeVarUInt(&result, inner.size()); result.append(inner);
    writeField(&result, Stop, 0); writeField(&result, Stop, 0); writeField(&result, Stop, 0);
    return result;
}

} // namespace eink
