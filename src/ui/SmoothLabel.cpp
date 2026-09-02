#include "SmoothLabel.h"

#include <QImage>
#include <QPainter>
#include <QtMath>

namespace eink {

SmoothLabel::SmoothLabel(QWidget *parent):QLabel(parent) {}
SmoothLabel::SmoothLabel(const QString &text,QWidget *parent):QLabel(text,parent) {}

void SmoothLabel::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    if(!pixmap(Qt::ReturnByValue).isNull()) { QLabel::paintEvent(event);return; }
    const qreal scale=devicePixelRatioF();
    QImage layer(qCeil(width()*scale),qCeil(height()*scale),QImage::Format_ARGB32_Premultiplied);
    layer.setDevicePixelRatio(scale);layer.fill(Qt::transparent);
    QPainter textPainter(&layer);
    textPainter.setRenderHint(QPainter::TextAntialiasing,true);
    textPainter.setFont(font());textPainter.setPen(palette().color(foregroundRole()));
    int flags=int(alignment())|Qt::TextDontClip;
    if(wordWrap())flags|=Qt::TextWordWrap;
    textPainter.drawText(contentsRect(),flags,text());
    textPainter.end();
    QPainter(this).drawImage(QPoint(0,0),layer);
}

} // namespace eink
