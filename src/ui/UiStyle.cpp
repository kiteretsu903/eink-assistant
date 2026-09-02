#include "UiStyle.h"

#include <QFrame>
#include <QImage>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QtMath>

namespace eink::ui {

SmoothProxyStyle::SmoothProxyStyle(const QString &baseStyleKey):QProxyStyle(baseStyleKey) {}

void SmoothProxyStyle::drawItemText(QPainter *painter,const QRect &rect,int flags,const QPalette &palette,
                                    bool enabled,const QString &text,QPalette::ColorRole textRole) const {
    if(text.isEmpty()||rect.isEmpty())return;
    const qreal scale=painter->device()->devicePixelRatioF();
    QImage layer(qCeil(rect.width()*scale),qCeil(rect.height()*scale),QImage::Format_ARGB32_Premultiplied);
    layer.setDevicePixelRatio(scale);layer.fill(Qt::transparent);
    QPainter textPainter(&layer);textPainter.setRenderHint(QPainter::TextAntialiasing,true);textPainter.setFont(painter->font());
    const QPalette::ColorGroup group=enabled?QPalette::Active:QPalette::Disabled;
    textPainter.setPen(textRole==QPalette::NoRole?painter->pen().color():palette.color(group,textRole));
    textPainter.setLayoutDirection(painter->layoutDirection());
    textPainter.drawText(QRect(QPoint(0,0),rect.size()),flags,text);textPainter.end();
    painter->drawImage(rect.topLeft(),layer);
}

QString styleSheet() {
    return QStringLiteral(R"(
        * { color: #111111; font-size: 15px; }
        QWidget { background: #ffffff; }
        QLabel[secondary="true"] { color: #4a4a4a; }
        QLabel[heading="true"] { font-size: 16px; font-weight: 600; }
        QPushButton { background: #ffffff; border: 2px solid #202020; border-radius: 6px; padding: 5px 9px; min-height: 20px; }
        QPushButton:hover { background: #f0f6ff; }
        QPushButton:pressed { background: #e5f1ff; }
        QPushButton:disabled { color: #777777; border-color: #777777; }
        QCheckBox { spacing: 8px; font-weight: 600; font-size: 16px; }
        QCheckBox::indicator { width: 24px; height: 24px; border: 2px solid #202020; border-radius: 4px; background: white; }
        QCheckBox::indicator:checked { background: #147ee5; border: 4px solid white; }
        QSlider::groove:horizontal { border: 2px solid #202020; height: 8px; background: #ffffff; border-radius: 5px; }
        QSlider::sub-page:horizontal { background: #147ee5; border-radius: 4px; }
        QSlider::handle:horizontal { background: #ffffff; border: 3px solid #202020; width: 22px; margin: -9px 0; border-radius: 13px; }
        QComboBox { background: #ffffff; border: 2px solid #202020; border-radius: 6px; padding: 5px 9px; min-width: 105px; }
        QComboBox::drop-down { border: 0; width: 24px; }
        QScrollArea { border: none; }
        QScrollBar:vertical { background: #f3f3f3; width: 12px; margin: 0; border: 0; }
        QScrollBar::handle:vertical { background: #686868; min-height: 52px; margin: 2px; border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: #303030; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; border: 0; background: transparent; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
        QScrollBar:horizontal { height: 0; }
        QToolTip { background: white; border: 2px solid #202020; color: #111111; padding: 5px; }
    )");
}

QFrame *divider() {
    auto *line=new QFrame; line->setFrameShape(QFrame::HLine); line->setFrameShadow(QFrame::Plain);
    line->setStyleSheet(QStringLiteral("QFrame { color:#202020; background:#202020; min-height:1px; max-height:1px; border:0; }")); return line;
}

QPushButton *outlinedButton(const QString &text, const QString &objectName) {
    auto *button=new QPushButton(text); if(!objectName.isEmpty())button->setObjectName(objectName);button->setFocusPolicy(Qt::TabFocus);button->setCursor(Qt::PointingHandCursor); return button;
}

void clearLayout(QLayout *layout) {
    while(QLayoutItem *item=layout->takeAt(0)) {
        if(QWidget *widget=item->widget()) {
            widget->hide();
            widget->deleteLater();
            delete item;
        } else if(QLayout *childLayout=item->layout()) {
            clearLayout(childLayout);
            delete childLayout;
        } else {
            delete item;
        }
    }
}

} // namespace eink::ui
