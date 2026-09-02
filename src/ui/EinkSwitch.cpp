#include "EinkSwitch.h"

#include <QPainter>
#include <QVariant>

namespace eink {

EinkSwitch::EinkSwitch(QWidget *parent):QAbstractButton(parent) {
    setCheckable(true);setFocusPolicy(Qt::TabFocus);setCursor(Qt::PointingHandCursor);setFixedSize(sizeHint());
    setAttribute(Qt::WA_TranslucentBackground);setAutoFillBackground(false);setProperty("antialiased-corners",true);
}

void EinkSwitch::paintEvent(QPaintEvent *) {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing,true);
    const QRectF track=QRectF(rect()).adjusted(1.75,1.75,-1.75,-1.75);
    p.setPen(QPen(isEnabled()?QColor("#202020"):QColor("#777777"),2.5,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.setBrush(Qt::white);p.drawRoundedRect(track,7.0,7.0);
    const QRectF thumb(isChecked()?width()-25.0:7.0,7.0,18.0,18.0);
    p.setPen(Qt::NoPen);p.setBrush(isChecked()?QColor("#147ee5"):QColor("#303030"));p.drawRoundedRect(thumb,2.0,2.0);
}

} // namespace eink
