#pragma once

#include <QRect>
#include <QString>
#include <QVector>
#include <QtGlobal>

namespace eink::windows {

enum class TrayPromotionResult { Unavailable, Pending, Overflow, Promoted };
enum class TrayPromotionStrategy { LegacyShellState, PerIconRegistry };

TrayPromotionStrategy trayPromotionStrategyForBuild(quint32 build);
TrayPromotionResult promoteOwnTrayIcon();
QVector<QRect> ownPromotedTrayIconRects();
bool ownTrayIconInOverflow();
QString trayIntegrationDiagnostic();

} // namespace eink::windows
