#include "TrayIcon.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QSettings>

namespace eink::ui {

QImage bookPagesTrayImage(int size, bool lightBackground) {
    QImage image(size,size,QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.scale(size/20.0,size/20.0);
    const QColor ink=lightBackground?QColor(20,20,20):QColor(245,245,245);
    QPen pen(ink,1.65,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    QPainterPath back;
    back.addRoundedRect(QRectF(6.0,2.0,11.0,13.0),1.8,1.8);
    painter.drawPath(back);
    painter.drawLine(QPointF(9.0,6.0),QPointF(14.0,6.0));
    painter.drawLine(QPointF(9.0,9.0),QPointF(14.0,9.0));
    painter.drawLine(QPointF(9.0,12.0),QPointF(13.0,12.0));

    QPainterPath front;
    front.addRoundedRect(QRectF(3.0,5.0,11.0,13.0),1.8,1.8);
    painter.save();
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillPath(front,Qt::transparent);
    painter.restore();
    painter.drawPath(front);
    painter.drawLine(QPointF(6.2,5.4),QPointF(6.2,17.6));
    painter.drawLine(QPointF(8.6,9.0),QPointF(11.8,9.0));
    painter.drawLine(QPointF(8.6,12.0),QPointF(11.8,12.0));
    painter.drawLine(QPointF(8.6,15.0),QPointF(11.2,15.0));
    return image;
}

QIcon bookPagesTrayIcon(bool lightBackground) {
    QIcon result;
    for(const int size:{16,20,24,32,40,48})
        result.addPixmap(QPixmap::fromImage(bookPagesTrayImage(size,lightBackground)));
    return result;
}

bool systemTrayUsesLightBackground() {
#ifdef Q_OS_WIN
    QSettings theme(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),QSettings::NativeFormat);
    return theme.value(QStringLiteral("SystemUsesLightTheme"),false).toInt()!=0;
#else
    return QGuiApplication::palette().color(QPalette::Window).lightness()>127;
#endif
}

} // namespace eink::ui
