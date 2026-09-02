#include "BusyDialog.h"
#include "UiStyle.h"

#include <QCursor>
#include <QGuiApplication>
#include <QHideEvent>
#include <QLabel>
#include <QPainter>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

namespace eink {
namespace {

class SpinnerWidget final : public QWidget {
public:
    explicit SpinnerWidget(QWidget *parent=nullptr):QWidget(parent) {
        setObjectName(QStringLiteral("busy-spinner"));setFixedSize(44,44);setProperty("angle",0);
        m_timer.setInterval(55);connect(&m_timer,&QTimer::timeout,this,[this]{m_step=(m_step+1)%12;setProperty("angle",m_step*30);update();});
    }
    void start(){m_timer.start();update();}
    void stop(){m_timer.stop();}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing,true);p.translate(width()/2.0,height()/2.0);
        QPen pen;pen.setWidthF(3.2);pen.setCapStyle(Qt::RoundCap);
        for(int i=0;i<12;++i){const int distance=(i-m_step+12)%12;const int gray=qBound(20,25+distance*19,225);pen.setColor(QColor(gray,gray,gray));p.setPen(pen);p.drawLine(QPointF(0,-11),QPointF(0,-18));p.rotate(30);}
    }
private:
    QTimer m_timer;
    int m_step=0;
};

} // namespace

BusyDialog::BusyDialog(QWidget *parent):QDialog(parent) {
    setObjectName(QStringLiteral("busy-dialog"));setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_TranslucentBackground);setFixedWidth(310);setStyleSheet(ui::styleSheet()+QStringLiteral("QDialog#busy-dialog{background:transparent;border:0;}QDialog#busy-dialog QLabel,QDialog#busy-dialog QWidget#busy-spinner{background:transparent;}"));
    auto *layout=new QVBoxLayout(this);layout->setContentsMargins(24,20,24,20);layout->setSpacing(10);layout->setAlignment(Qt::AlignCenter);
    m_spinner=new SpinnerWidget(this);layout->addWidget(m_spinner,0,Qt::AlignHCenter);
    m_message=new QLabel(this);m_message->setObjectName(QStringLiteral("busy-message"));m_message->setAlignment(Qt::AlignCenter);m_message->setWordWrap(true);m_message->setMinimumHeight(24);layout->addWidget(m_message);
}

void BusyDialog::setMessage(const QString &message){m_message->setText(message);adjustSize();}

void BusyDialog::showCentered() {
    QScreen *screen=QGuiApplication::screenAt(QCursor::pos());if(!screen)screen=QGuiApplication::primaryScreen();if(screen){const QRect area=screen->availableGeometry();move(area.center()-rect().center());}
    show();raise();activateWindow();
}

void BusyDialog::showEvent(QShowEvent *event){QDialog::showEvent(event);static_cast<SpinnerWidget*>(m_spinner)->start();}
void BusyDialog::hideEvent(QHideEvent *event){static_cast<SpinnerWidget*>(m_spinner)->stop();QDialog::hideEvent(event);}
void BusyDialog::paintEvent(QPaintEvent *event){Q_UNUSED(event) QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);painter.setPen(QPen(QColor(QStringLiteral("#202020")),2));painter.setBrush(Qt::white);painter.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),9,9);}

} // namespace eink
