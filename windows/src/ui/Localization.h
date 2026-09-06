#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QObject>
#include <QFont>

namespace eink {

struct LocaleInfo {
    QString code;
    QString name;
    bool rightToLeft = false;
};

class Localization : public QObject {
    Q_OBJECT
public:
    static Localization &instance();
    void setLanguage(const QString &language);
    QString language() const { return m_language; }
    QString text(const char *key) const;
    static QString systemLanguage();
    static QFont fontForLanguage(const QString &code);
    static const QVector<LocaleInfo> &locales();
    static QString resolveLanguage(const QStringList &preferences);
    static QHash<QString, QString> parseCatalog(const QByteArray &data);
    static QHash<QString, QString> catalog(const QString &code, bool windows = false);
    QString resolvedLanguage() const { return m_resolvedLanguage; }

signals:
    void languageChanged();

private:
    Localization();
    void load(const QString &language);
    QString m_language;
    QString m_resolvedLanguage;
    QHash<QString, QString> m_english;
    QHash<QString, QString> m_current;
};

inline QString L(const char *key) { return Localization::instance().text(key); }

} // namespace eink
