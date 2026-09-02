#include "WelcomeDialog.h"
#include "app/ApplicationController.h"
#include "Localization.h"
#include "SmoothLabel.h"
#include "UiStyle.h"

#include <QCheckBox>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QPointer>
#include <QRegion>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

namespace eink {

WelcomeDialog::WelcomeDialog(ApplicationController *controller,QWidget *parent)
    :QDialog(parent),m_controller(controller) {
    setWindowTitle(L("welcome.title"));
    setWindowIcon(QIcon(QStringLiteral(":/app-icon.png")));
    setModal(false);
    setObjectName(QStringLiteral("tray-welcome"));
    setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setMinimumWidth(620);
    setStyleSheet(ui::styleSheet()+QStringLiteral("QDialog#tray-welcome{background:transparent;border:0;}"));
    auto *root=new QVBoxLayout(this);root->setContentsMargins(28,26,28,40);root->setSpacing(16);
    auto *titleRow=new QHBoxLayout;
    auto *icon=new QLabel;icon->setPixmap(QIcon(QStringLiteral(":/app-icon.png")).pixmap(64,64));titleRow->addWidget(icon);
    auto *title=new SmoothLabel(L("welcome.title"));QFont titleFont=title->font();titleFont.setPointSize(19);titleFont.setWeight(QFont::DemiBold);title->setFont(titleFont);title->setWordWrap(true);titleRow->addWidget(title,1);root->addLayout(titleRow);
    auto addTip=[&](const QString &heading,const QString &body){
        auto *card=new QFrame;card->setStyleSheet(QStringLiteral("QFrame{border:2px solid #202020;border-radius:7px;background:white;}QLabel{border:0;}"));
        auto *layout=new QVBoxLayout(card);auto *head=new SmoothLabel(heading);QFont f=head->font();f.setWeight(QFont::DemiBold);head->setFont(f);layout->addWidget(head);
        QString plainBody=body;plainBody.remove(QStringLiteral("**"));
        auto *text=new SmoothLabel(plainBody);text->setWordWrap(true);text->setStyleSheet(QStringLiteral("color:#4a4a4a;border:0;"));layout->addWidget(text);root->addWidget(card);
    };
    addTip(L("welcome.windows.tray.title"),L("welcome.windows.tray"));
    addTip(L("welcome.bigme.title"),L("welcome.bigme.body"));
    addTip(L("welcome.other.title"),L("welcome.windows.other"));
    auto *bottom=new QHBoxLayout;auto *hide=new QCheckBox(L("welcome.hide"));hide->setObjectName(QStringLiteral("welcome-hide"));bottom->addWidget(hide);bottom->addStretch();
    auto *done=ui::outlinedButton(L("welcome.done"),QStringLiteral("welcome-done"));bottom->addWidget(done);root->addLayout(bottom);
    connect(done,&QPushButton::clicked,this,[this,hide]{if(hide->isChecked())m_controller->setShowWelcome(false);accept();});
}

void WelcomeDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);adjustSize();positionNearNotificationArea();raise();
    const QPointer<WelcomeDialog> self(this);QTimer::singleShot(80,this,[self]{if(self&&self->isVisible())self->positionNearNotificationArea();});
}

void WelcomeDialog::setTrayAnchorRect(const QRect &rect) {
    m_trayAnchorRect=rect;
    if(QScreen *screen=QGuiApplication::screenAt(rect.center())){if(!windowHandle())winId();if(QWindow *window=windowHandle();window&&window->screen()!=screen)window->setScreen(screen);}
    if(isVisible())positionNearNotificationArea();
}

void WelcomeDialog::positionNearNotificationArea() {
    const bool anchored=m_trayAnchorRect.isValid()&&!m_trayAnchorRect.isEmpty();
    QScreen *screen=anchored?QGuiApplication::screenAt(m_trayAnchorRect.center()):QGuiApplication::primaryScreen();
    if(!screen)screen=QGuiApplication::primaryScreen();if(!screen)return;
    if(!windowHandle())winId();if(QWindow *window=windowHandle();window&&window->screen()!=screen)window->setScreen(screen);
    const QRect full=screen->geometry(),available=screen->availableGeometry();
    const int left=available.left()-full.left(),top=available.top()-full.top();
    const int right=full.right()-available.right(),bottom=full.bottom()-available.bottom();
    const int margin=14;
    const int arrowInset=62;
    auto clampStart=[](int wanted,int low,int high){if(high<low)return low;return qBound(low,wanted,high);};
    if(bottom>=top&&bottom>=left&&bottom>=right) {
        const int targetX=anchored?m_trayAnchorRect.center().x():available.right()-arrowInset;
        const int windowX=clampStart(targetX-(width()-arrowInset),available.left()+margin,available.right()-width()-margin+1);
        m_arrowEdge=ArrowEdge::Bottom;m_arrowPosition=qBound<qreal>(30,targetX-windowX,width()-30);
        move(windowX,available.bottom()-height()+3);
    } else if(top>=left&&top>=right) {
        const int targetX=anchored?m_trayAnchorRect.center().x():available.right()-arrowInset;
        const int windowX=clampStart(targetX-(width()-arrowInset),available.left()+margin,available.right()-width()-margin+1);
        m_arrowEdge=ArrowEdge::Top;m_arrowPosition=qBound<qreal>(30,targetX-windowX,width()-30);
        move(windowX,available.top()-3);
    } else if(right>=left) {
        const int targetY=anchored?m_trayAnchorRect.center().y():available.bottom()-arrowInset;
        const int windowY=clampStart(targetY-(height()-arrowInset),available.top()+margin,available.bottom()-height()-margin+1);
        m_arrowEdge=ArrowEdge::Right;m_arrowPosition=qBound<qreal>(30,targetY-windowY,height()-30);
        move(available.right()-width()+3,windowY);
    } else {
        const int targetY=anchored?m_trayAnchorRect.center().y():available.bottom()-arrowInset;
        const int windowY=clampStart(targetY-(height()-arrowInset),available.top()+margin,available.bottom()-height()-margin+1);
        m_arrowEdge=ArrowEdge::Left;m_arrowPosition=qBound<qreal>(30,targetY-windowY,height()-30);
        move(available.left()-3,windowY);
    }
    update();
}

void WelcomeDialog::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    constexpr qreal arrow=14,radius=11,pad=3,halfBase=13;
    QRectF body(pad,pad,width()-2*pad,height()-2*pad);
    if(m_arrowEdge==ArrowEdge::Bottom)body.adjust(0,0,0,-arrow);
    else if(m_arrowEdge==ArrowEdge::Top)body.adjust(0,arrow,0,0);
    else if(m_arrowEdge==ArrowEdge::Right)body.adjust(0,0,-arrow,0);
    else body.adjust(arrow,0,0,0);
    QPainterPath shape;shape.addRoundedRect(body,radius,radius);QPolygonF triangle;
    if(m_arrowEdge==ArrowEdge::Bottom)triangle<<QPointF(m_arrowPosition-halfBase,body.bottom())<<QPointF(m_arrowPosition,height()-pad)<<QPointF(m_arrowPosition+halfBase,body.bottom());
    else if(m_arrowEdge==ArrowEdge::Top)triangle<<QPointF(m_arrowPosition-halfBase,body.top())<<QPointF(m_arrowPosition,pad)<<QPointF(m_arrowPosition+halfBase,body.top());
    else if(m_arrowEdge==ArrowEdge::Right)triangle<<QPointF(body.right(),m_arrowPosition-halfBase)<<QPointF(width()-pad,m_arrowPosition)<<QPointF(body.right(),m_arrowPosition+halfBase);
    else triangle<<QPointF(body.left(),m_arrowPosition-halfBase)<<QPointF(pad,m_arrowPosition)<<QPointF(body.left(),m_arrowPosition+halfBase);
    shape.addPolygon(triangle);shape.closeSubpath();
    QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setPen(QPen(QColor(QStringLiteral("#202020")),2));painter.setBrush(Qt::white);painter.drawPath(shape);
}

} // namespace eink
