#pragma once

#include <QByteArray>
#include <QRect>
#include <QString>
#include <QVector>
#include <QtGlobal>
#include <memory>

namespace eink::windows {

enum class TrayPromotionResult { Unavailable, Pending, Overflow, Promoted };
enum class TrayPromotionStrategy { LegacyCom, ClassicIconStreams, PerIconRegistry };

class TrayPromotionWatcher final {
public:
    TrayPromotionWatcher();
    ~TrayPromotionWatcher();
    TrayPromotionWatcher(const TrayPromotionWatcher &) = delete;
    TrayPromotionWatcher &operator=(const TrayPromotionWatcher &) = delete;

    // Arm before QSystemTrayIcon::show(). On Windows 11, cleanStaleEntries
    // removes obsolete same-filename identities but preserves the exact live
    // entry and any visibility choice already made by the user.
    bool arm(bool cleanStaleEntries);
    void stop();
    TrayPromotionResult result() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

TrayPromotionStrategy trayPromotionStrategyForBuild(quint32 build);
bool patchClassicIconStreamsBlobForExecutable(QByteArray *data,const QString &executable,
                                               bool *matched=nullptr,bool *changed=nullptr);
TrayPromotionResult promoteOwnTrayIcon();
QVector<QRect> ownPromotedTrayIconRects();
bool ownTrayIconInOverflow();
QString trayIntegrationDiagnostic();

} // namespace eink::windows
