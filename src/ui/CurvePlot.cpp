#include "CurvePlot.h"

#include <QPainter>
#include <QPainterPath>

namespace eink {

CurvePlot::CurvePlot(QWidget *parent):QWidget(parent) { setMinimumHeight(96); setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed); }

void CurvePlot::paintEvent(QPaintEvent *) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing,true); QRectF r=rect().adjusted(1,1,-1,-1);
    p.fillRect(r,Qt::white); p.setPen(QPen(QColor("#202020"),2)); p.drawRect(r);
    p.setPen(QPen(QColor("#d0d0d0"),1));
    for(int i=1;i<4;++i){const qreal x=r.left()+r.width()*i/4.0; const qreal y=r.top()+r.height()*i/4.0; p.drawLine(QPointF(x,r.top()),QPointF(x,r.bottom()));p.drawLine(QPointF(r.left(),y),QPointF(r.right(),y));}
    p.setPen(QPen(QColor("#8a8a8a"),1,Qt::DashLine)); p.drawLine(r.bottomLeft(),r.topRight());
    QPainterPath path;
    for(int i=0;i<=192;++i){const double x=i/192.0,y=m_curve.value(x);const QPointF point(r.left()+x*r.width(),r.bottom()-y*r.height());if(i==0)path.moveTo(point);else path.lineTo(point);}
    p.setPen(QPen(QColor("#147ee5"),3)); p.drawPath(path);
}

} // namespace eink
