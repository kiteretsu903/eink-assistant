#pragma once

#include <QHash>
#include <QString>

namespace eink {

class Localization {
public:
    static Localization &instance();
    void setLanguage(const QString &language);
    QString language() const { return m_language; }
    QString text(const char *key) const;
    static QString systemLanguage();

private:
    Localization();
    void load(const QString &language);
    QString m_language;
    QHash<QString, QString> m_english;
    QHash<QString, QString> m_current;
};

inline QString L(const char *key) { return Localization::instance().text(key); }

} // namespace eink
