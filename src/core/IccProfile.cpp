#include "IccProfile.h"

#include <QtEndian>
#include <QHash>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace eink {
namespace {

quint32 readU32(const QByteArray &b, int offset) {
    return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(b.constData() + offset));
}

double readS15(const QByteArray &b, int offset) {
    const qint32 raw = static_cast<qint32>(readU32(b, offset));
    return static_cast<double>(raw) / 65536.0;
}

void putU16(QByteArray &b, quint16 value) {
    const quint16 be = qToBigEndian(value);
    b.append(reinterpret_cast<const char *>(&be), 2);
}

void putU32(QByteArray &b, quint32 value) {
    const quint32 be = qToBigEndian(value);
    b.append(reinterpret_cast<const char *>(&be), 4);
}

void setU32(QByteArray &b, int offset, quint32 value) {
    const quint32 be = qToBigEndian(value);
    std::memcpy(b.data() + offset, &be, 4);
}

void putS15(QByteArray &b, double value) {
    const double bounded = std::max(-32768.0, std::min(value, 32767.9999847412));
    putU32(b, static_cast<quint32>(static_cast<qint32>(std::llround(bounded * 65536.0))));
}

QByteArray xyzTag(double x, double y, double z) {
    QByteArray tag("XYZ ", 4);
    putU32(tag, 0);
    putS15(tag, x); putS15(tag, y); putS15(tag, z);
    return tag;
}

QByteArray gammaTag(double gamma) {
    QByteArray tag("para", 4);
    putU32(tag, 0);
    putU16(tag, 0);
    putU16(tag, 0);
    putS15(tag, gamma);
    return tag;
}

QByteArray mlucTag(const QString &text) {
    QByteArray utf16;
    utf16.reserve(text.size() * 2);
    for (QChar c : text) putU16(utf16, c.unicode());
    QByteArray tag("mluc", 4);
    putU32(tag, 0);
    putU32(tag, 1);
    putU32(tag, 12);
    tag.append("enUS", 4);
    putU32(tag, static_cast<quint32>(utf16.size()));
    putU32(tag, 28);
    tag.append(utf16);
    return tag;
}

struct Tag { QByteArray signature; QByteArray payload; };

QByteArray mhc2Tag(double saturation, const RgbBalance &balance) {
    constexpr double rgbToXyz[9] = {
        0.4123907993, 0.3575843394, 0.1804807884,
        0.2126390059, 0.7151686788, 0.0721923154,
        0.0193308187, 0.1191947798, 0.9505321522
    };
    constexpr double xyzToRgb[9] = {
         3.2409699419, -1.5373831776, -0.4986107603,
        -0.9692436363,  1.8759675015,  0.0415550574,
         0.0556300797, -0.2039769589,  1.0569715142
    };
    constexpr double luma[3] = {0.2126, 0.7152, 0.0722};
    const double s = std::max(0.0, std::min(saturation, 3.0));
    const double gains[3] = {
        std::max(0.0, std::min(balance.red, 2.0)),
        std::max(0.0, std::min(balance.green, 2.0)),
        std::max(0.0, std::min(balance.blue, 2.0))
    };
    double rgbAdjust[9] = {};
    for (int row=0; row<3; ++row)
        for (int col=0; col<3; ++col)
            rgbAdjust[row*3+col] = gains[row] * ((row==col ? s : 0.0) + (1.0-s)*luma[col]);
    auto multiply=[](const double a[9],const double b[9],double out[9]) {
        for(int row=0;row<3;++row)for(int col=0;col<3;++col) {
            out[row*3+col]=0;
            for(int k=0;k<3;++k)out[row*3+col]+=a[row*3+k]*b[k*3+col];
        }
    };
    double first[9]{}, xyzAdjust[9]{};
    multiply(rgbToXyz,rgbAdjust,first);
    multiply(first,xyzToRgb,xyzAdjust);

    QByteArray tag("MHC2",4);
    putU32(tag,0);                 // reserved
    putU32(tag,2);                 // two-point identity 1DLUT
    putS15(tag,0.05);              // valid ST.2086 minimum luminance
    putS15(tag,300.0);             // conservative generic peak luminance
    putU32(tag,36);                // matrix offset, relative to this tag
    putU32(tag,84); putU32(tag,100); putU32(tag,116);
    for(int row=0;row<3;++row) {
        for(int col=0;col<3;++col) putS15(tag,xyzAdjust[row*3+col]);
        putS15(tag,0.0);           // fourth column is reserved/ignored
    }
    for(int channel=0;channel<3;++channel) {
        tag.append("sf32",4);putU32(tag,0);putS15(tag,0.0);putS15(tag,1.0);
    }
    return tag;
}

bool findTag(const QByteArray &b,const QByteArray &signature,quint32 *offset,quint32 *size) {
    if(b.size()<132)return false;
    const quint32 count=readU32(b,128);
    if(132ull+count*12ull>static_cast<quint64>(b.size()))return false;
    for(quint32 i=0;i<count;++i) {
        const int entry=132+static_cast<int>(i)*12;
        if(b.mid(entry,4)!=signature)continue;
        const quint32 foundOffset=readU32(b,entry+4),foundSize=readU32(b,entry+8);
        if(foundOffset+foundSize>static_cast<quint32>(b.size()))return false;
        if(offset)*offset=foundOffset;if(size)*size=foundSize;return true;
    }
    return false;
}

} // namespace

QByteArray IccProfile::make(double saturation, const RgbBalance &balance,
                            const QString &description, const IccBaseProfile &base) {
    QVector<Tag> tags {
        {"desc", mlucTag(description)},
        {"cprt", mlucTag(QStringLiteral("E-Ink Assistant - MIT licensed"))},
        {"wtpt", xyzTag(0.95047,1.0,1.08883)},
        {"bkpt", xyzTag(0.0,0.0,0.0)},
        {"rXYZ", xyzTag(base.matrix[0], base.matrix[3], base.matrix[6])},
        {"gXYZ", xyzTag(base.matrix[1], base.matrix[4], base.matrix[7])},
        {"bXYZ", xyzTag(base.matrix[2], base.matrix[5], base.matrix[8])},
        {"rTRC", gammaTag(base.gamma)},
        {"gTRC", gammaTag(base.gamma)},
        {"bTRC", gammaTag(base.gamma)},
        {"lumi", xyzTag(285.141,300.0,326.649)},
        {"MHC2", mhc2Tag(saturation,balance)}
    };

    QByteArray header(128, '\0');
    setU32(header, 8, 0x04300000);
    std::memcpy(header.data()+12, "mntr", 4);
    std::memcpy(header.data()+16, "RGB ", 4);
    std::memcpy(header.data()+20, "XYZ ", 4);
    QByteArray date;
    putU16(date, 2026); putU16(date, 8); putU16(date, 31);
    putU16(date, 0); putU16(date, 0); putU16(date, 0);
    std::memcpy(header.data()+24, date.constData(), 12);
    std::memcpy(header.data()+36, "acsp", 4);
    std::memcpy(header.data()+40, "MSFT", 4);
    setU32(header, 64, 1);
    QByteArray illum; putS15(illum, 0.9642); putS15(illum, 1.0); putS15(illum, 0.8249);
    std::memcpy(header.data()+68, illum.constData(), 12);
    std::memcpy(header.data()+80, "EINK", 4);

    QByteArray table;
    putU32(table, static_cast<quint32>(tags.size()));
    QByteArray body;
    const int bodyStart = 128 + 4 + tags.size() * 12;
    QHash<QByteArray, int> shared;
    for (const Tag &tag : tags) {
        table.append(tag.signature);
        int offset = shared.value(tag.payload, -1);
        if (offset < 0) {
            offset = bodyStart + body.size();
            shared.insert(tag.payload, offset);
            body.append(tag.payload);
            while (body.size() % 4) body.append('\0');
        }
        putU32(table, static_cast<quint32>(offset));
        putU32(table, static_cast<quint32>(tag.payload.size()));
    }
    QByteArray out = header + table + body;
    setU32(out, 0, static_cast<quint32>(out.size()));
    return out;
}

bool IccProfile::parseBase(const QByteArray &b, IccBaseProfile *base) {
    if (!base || b.size() < 132 || readU32(b, 0) > static_cast<quint32>(b.size())) return false;
    const quint32 count = readU32(b, 128);
    if (count == 0 || 132ull + count*12ull > static_cast<quint64>(b.size())) return false;
    bool haveR=false, haveG=false, haveB=false, haveW=false;
    double r[3]{}, g[3]{}, bl[3]{}, w[3]{};
    for (quint32 i=0; i<count; ++i) {
        const int e = 132 + static_cast<int>(i)*12;
        const QByteArray sig = b.mid(e,4);
        const quint32 off=readU32(b,e+4), size=readU32(b,e+8);
        if (off+size > static_cast<quint32>(b.size()) || size < 12) continue;
        auto readXyz = [&](double *v){ if (size >= 20) { v[0]=readS15(b,off+8); v[1]=readS15(b,off+12); v[2]=readS15(b,off+16); return true; } return false; };
        if (sig=="rXYZ") haveR=readXyz(r);
        else if (sig=="gXYZ") haveG=readXyz(g);
        else if (sig=="bXYZ") haveB=readXyz(bl);
        else if (sig=="wtpt") haveW=readXyz(w);
        else if (sig=="rTRC" && b.mid(off,4)=="para" && size>=16) base->gamma=readS15(b,off+12);
        else if (sig=="rTRC" && b.mid(off,4)=="curv" && size>=14 && readU32(b,off+8)==1)
            base->gamma=qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(b.constData()+off+12))/256.0;
    }
    if (!(haveR&&haveG&&haveB&&haveW)) return false;
    const double matrix[9]={r[0],g[0],bl[0],r[1],g[1],bl[1],r[2],g[2],bl[2]};
    std::copy(matrix,matrix+9,base->matrix); std::copy(w,w+3,base->whitePoint);
    return true;
}

QString IccProfile::description(const QByteArray &b) {
    if (b.size()<132) return {};
    const quint32 count=readU32(b,128);
    if (132ull+count*12ull>static_cast<quint64>(b.size())) return {};
    for (quint32 i=0;i<count;++i) {
        const int e=132+static_cast<int>(i)*12;
        if (b.mid(e,4)!="desc") continue;
        const quint32 off=readU32(b,e+4), size=readU32(b,e+8);
        if (off+size>static_cast<quint32>(b.size()) || size<28 || b.mid(off,4)!="mluc") return {};
        const quint32 len=readU32(b,off+20), strOff=readU32(b,off+24);
        if (off+strOff+len>static_cast<quint32>(b.size()) || len%2) return {};
        QString text; text.reserve(len/2);
        for (quint32 p=0;p<len;p+=2)
            text.append(QChar(qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(b.constData()+off+strOff+p))));
        return text;
    }
    return {};
}

bool IccProfile::structurallyValid(const QByteArray &b, QString *error) {
    auto fail=[&](const QString &e){ if(error)*error=e; return false; };
    if (b.size()<132) return fail(QStringLiteral("profile is shorter than the ICC header"));
    if (readU32(b,0)!=static_cast<quint32>(b.size())) return fail(QStringLiteral("header size does not match file size"));
    if (b.mid(36,4)!="acsp" || b.mid(12,4)!="mntr" || b.mid(16,4)!="RGB ") return fail(QStringLiteral("invalid display profile header"));
    const quint32 count=readU32(b,128);
    if (count<8 || 132ull+count*12ull>static_cast<quint64>(b.size())) return fail(QStringLiteral("invalid tag table"));
    for (quint32 i=0;i<count;++i) {
        const int e=132+static_cast<int>(i)*12;
        const quint32 off=readU32(b,e+4), size=readU32(b,e+8);
        if (off%4 || off+size>static_cast<quint32>(b.size())) return fail(QStringLiteral("tag points outside profile"));
    }
    IccBaseProfile base;
    if (!parseBase(b,&base)) return fail(QStringLiteral("matrix/TRC tags cannot be parsed"));
    if (description(b).isEmpty()) return fail(QStringLiteral("description tag cannot be parsed"));
    quint32 mhcOffset=0,mhcSize=0,lumiOffset=0,lumiSize=0,whiteOffset=0,whiteSize=0;
    if(!findTag(b,"wtpt",&whiteOffset,&whiteSize)||whiteSize<20||std::abs(readS15(b,whiteOffset+8)-0.95047)>0.001||std::abs(readS15(b,whiteOffset+12)-1.0)>0.001||std::abs(readS15(b,whiteOffset+16)-1.08883)>0.001)
        return fail(QStringLiteral("D65 display white-point metadata is invalid"));
    if(!findTag(b,"lumi",&lumiOffset,&lumiSize)||lumiSize<20||b.mid(lumiOffset,4)!="XYZ ")
        return fail(QStringLiteral("luminance metadata is missing"));
    if(std::abs(readS15(b,lumiOffset+8)-285.141)>0.01||std::abs(readS15(b,lumiOffset+12)-300.0)>0.01||std::abs(readS15(b,lumiOffset+16)-326.649)>0.01)
        return fail(QStringLiteral("D65 luminance metadata is invalid"));
    if(!findTag(b,"MHC2",&mhcOffset,&mhcSize)||mhcSize<132||b.mid(mhcOffset,4)!="MHC2")
        return fail(QStringLiteral("MHC2 hardware calibration tag is missing"));
    if(readU32(b,mhcOffset+8)!=2||readU32(b,mhcOffset+20)!=36||readU32(b,mhcOffset+24)!=84||readU32(b,mhcOffset+28)!=100||readU32(b,mhcOffset+32)!=116)
        return fail(QStringLiteral("MHC2 matrix offset is invalid"));
    for(const int lutOffset:{84,100,116})if(b.mid(mhcOffset+lutOffset,4)!="sf32"||std::abs(readS15(b,mhcOffset+lutOffset+8))>0.00001||std::abs(readS15(b,mhcOffset+lutOffset+12)-1.0)>0.00001)
        return fail(QStringLiteral("MHC2 identity LUT is invalid"));
    return true;
}

double IccProfile::matrixDeterminant(const QByteArray &data) {
    IccBaseProfile base;
    if (!parseBase(data,&base)) return 0.0;
    const double *m=base.matrix;
    return m[0]*(m[4]*m[8]-m[5]*m[7])-m[1]*(m[3]*m[8]-m[5]*m[6])+m[2]*(m[3]*m[7]-m[4]*m[6]);
}

bool IccProfile::mhc2Matrix(const QByteArray &data,double matrix[9]) {
    if(!matrix)return false;
    quint32 offset=0,size=0;
    if(!findTag(data,"MHC2",&offset,&size)||size<132||data.mid(offset,4)!="MHC2")return false;
    const quint32 matrixOffset=readU32(data,offset+20);
    if(matrixOffset<36||matrixOffset+48>size)return false;
    const int start=static_cast<int>(offset+matrixOffset);
    for(int row=0;row<3;++row)for(int col=0;col<3;++col)
        matrix[row*3+col]=readS15(data,start+(row*4+col)*4);
    return true;
}

} // namespace eink
