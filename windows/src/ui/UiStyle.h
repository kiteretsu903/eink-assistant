#pragma once

#include <QProxyStyle>
#include <QString>

class QFrame;
class QLayout;
class QPushButton;

namespace eink::ui {

class SmoothProxyStyle final : public QProxyStyle {
public:
    explicit SmoothProxyStyle(const QString &baseStyleKey);
    void drawPrimitive(PrimitiveElement element,const QStyleOption *option,QPainter *painter,
                       const QWidget *widget=nullptr) const override;
    void drawItemText(QPainter *painter,const QRect &rect,int flags,const QPalette &palette,
                      bool enabled,const QString &text,QPalette::ColorRole textRole=QPalette::NoRole) const override;
};

QString styleSheet();
QFrame *divider();
QPushButton *outlinedButton(const QString &text, const QString &objectName = {});
void clearLayout(QLayout *layout);

} // namespace eink::ui
