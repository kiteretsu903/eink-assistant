#include "Localization.h"

#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>
#include <algorithm>

static void initializeEinkResources() { Q_INIT_RESOURCE(resources); }

namespace eink {
namespace {
QFont windowsUiFont(const QString &language) {
    if (!qobject_cast<QApplication *>(QCoreApplication::instance())) return QFont();
    QFont font=QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    QStringList candidates;
    if(language=="zh-Hans") candidates=QStringList{"Microsoft YaHei UI","Microsoft YaHei"};
    else if(language=="zh-Hant") candidates=QStringList{"Microsoft JhengHei UI","Microsoft JhengHei"};
    else if(language=="ja") candidates=QStringList{"Yu Gothic UI","Meiryo UI","Meiryo"};
    else if(language=="ko") candidates=QStringList{"Malgun Gothic","Gulim"};
    else {
        // Only choose installed families; Qt keeps its glyph fallback enabled.
        const QHash<QString,QStringList> scripts {
            {"ar",{"Segoe UI","Tahoma"}},{"fa",{"Segoe UI","Tahoma"}},{"he",{"Segoe UI","Tahoma"}},
            {"ur",{"Nirmala UI","Urdu Typesetting","Tahoma"}},
            {"hi",{"Nirmala UI","Mangal"}},{"mr",{"Nirmala UI","Mangal"}},{"ne",{"Nirmala UI","Mangal"}},
            {"bn",{"Nirmala UI","Vrinda"}},{"gu",{"Nirmala UI","Shruti"}},{"pa",{"Nirmala UI","Raavi"}},
            {"ta",{"Nirmala UI","Latha"}},{"te",{"Nirmala UI","Gautami"}},{"kn",{"Nirmala UI","Tunga"}},
            {"ml",{"Nirmala UI","Kartika"}},{"si",{"Nirmala UI","Iskoola Pota"}},
            {"th",{"Leelawadee UI","Leelawadee","Tahoma"}},{"lo",{"Leelawadee UI","DokChampa"}},
            {"km",{"Leelawadee UI","Khmer UI","DaunPenh"}},{"my",{"Myanmar Text"}},
            {"am",{"Nyala"}},{"hy",{"Sylfaen"}},{"ka",{"Sylfaen"}}
        };
        candidates=scripts.value(language);
        candidates << "Segoe UI" << font.family() << "Tahoma";
    }
    static const QStringList installed=QFontDatabase().families();
    for(const QString &candidate:candidates) {
        auto match=std::find_if(installed.cbegin(),installed.cend(),[&](const QString &family){return family.compare(candidate,Qt::CaseInsensitive)==0;});
        if(match!=installed.cend()){font.setFamily(*match);break;}
    }
    font.setStyleStrategy(static_cast<QFont::StyleStrategy>(QFont::PreferAntialias|QFont::PreferQuality));
    font.setHintingPreference(QFont::PreferNoHinting);
    return font;
}
void merge(QHash<QString,QString> &target,const QHash<QString,QString> &source) {
    for(auto it=source.cbegin();it!=source.cend();++it)target.insert(it.key(),it.value());
}
} // namespace

QHash<QString,QString> Localization::parseCatalog(const QByteArray &data) {
    // The shared generator emits JSON string literals in an Apple .strings wrapper.
    // Decode each literal with Qt's JSON decoder, including escapes and surrogate pairs.
    QHash<QString,QString> result;
    const QString content=QString::fromUtf8(data);
    const QRegularExpression re(QStringLiteral(R"re(("(?:\\.|[^"\\])*")\s*=\s*("(?:\\.|[^"\\])*")\s*;)re"));
    auto matches=re.globalMatch(content);
    while(matches.hasNext()) {
        const auto match=matches.next();
        const auto pair=QJsonDocument::fromJson(("["+match.captured(1)+","+match.captured(2)+"]").toUtf8()).array();
        if(pair.size()!=2||!pair[0].isString()||!pair[1].isString()||result.contains(pair[0].toString()))return {};
        result.insert(pair[0].toString(),pair[1].toString());
    }
    return result;
}
QHash<QString,QString> Localization::catalog(const QString &code,bool windows) {
    initializeEinkResources();
    QFile file(QStringLiteral(":/i18n/%1%2.strings").arg(windows?QStringLiteral("windows/"):QString(),code));
    if(!file.open(QIODevice::ReadOnly))return {};
    return parseCatalog(file.readAll());
}
const QVector<LocaleInfo> &Localization::locales() {
    static const QVector<LocaleInfo> registry=[]{
        initializeEinkResources();
        QFile file(QStringLiteral(":/i18n/locales.json"));
        QVector<LocaleInfo> result;
        if(!file.open(QIODevice::ReadOnly))return result;
        for(const auto &value:QJsonDocument::fromJson(file.readAll()).array()) {
            const auto item=value.toObject();
            result.append({item["code"].toString(),item["name"].toString(),item["dir"].toString()=="rtl"});
        }
        return result;
    }();
    return registry;
}
QString Localization::resolveLanguage(const QStringList &preferences) {
    const auto exact=[](const QString &code){
        for(const auto &locale:locales())if(locale.code.compare(code,Qt::CaseInsensitive)==0)return locale.code;
        return QString();
    };
    const QRegularExpression valid(QStringLiteral("^[a-zA-Z]{2,3}(?:-[a-zA-Z0-9]{2,8})*$"));
    for(QString tag:preferences) {
        tag.replace('_','-');
        if(!valid.match(tag).hasMatch())continue;
        if(const QString match=exact(tag);!match.isEmpty())return match;
        const QStringList parts=tag.toLower().split('-');
        const QString base=parts.first();
        if(base=="zh") {
            if(parts.contains("hant"))return "zh-Hant";
            if(parts.contains("hans"))return "zh-Hans";
            return parts.contains("tw")||parts.contains("hk")||parts.contains("mo")?"zh-Hant":"zh-Hans";
        }
        if(base=="pt")return parts.contains("br")?"pt-BR":"pt-PT";
        // Qt 5 may not know a registry locale; matching never depends on its enum.
        if(const QString match=exact(base);!match.isEmpty())return match;
    }
    return "en";
}
QFont Localization::fontForLanguage(const QString &code) {return windowsUiFont(code);}
Localization &Localization::instance() {static Localization value;return value;}
Localization::Localization() {
    m_english=catalog("en");merge(m_english,catalog("en",true));
    setLanguage("system");
}
QString Localization::systemLanguage() {return resolveLanguage(QLocale::system().uiLanguages());}
void Localization::setLanguage(const QString &language) {
    m_language=language;
    m_resolvedLanguage=language=="system"?systemLanguage():resolveLanguage({language});
    load(m_resolvedLanguage);
    if(qobject_cast<QApplication *>(QCoreApplication::instance()))QApplication::setFont(windowsUiFont(m_resolvedLanguage));
    if(qobject_cast<QApplication *>(QCoreApplication::instance())) {
        bool rtl=false;
        for(const auto &locale:locales())if(locale.code==m_resolvedLanguage)rtl=locale.rightToLeft;
        QApplication::setLayoutDirection(rtl?Qt::RightToLeft:Qt::LeftToRight);
    }
    emit languageChanged();
}
void Localization::load(const QString &language) {
    m_current=m_english;
    merge(m_current,catalog(language));
    merge(m_current,catalog(language,true));
}
QString Localization::text(const char *key) const {
    const QString value=QString::fromLatin1(key);
    return m_current.value(value,value);
}
} // namespace eink
