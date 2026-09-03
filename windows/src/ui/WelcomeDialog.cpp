#include "WelcomeDialog.h"
#include "app/ApplicationController.h"
#include "Localization.h"
#include "SmoothLabel.h"
#include "TrayIcon.h"
#include "UiStyle.h"

#include <QCheckBox>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QPointer>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

namespace eink {
namespace {

class WelcomeGlyph final : public QWidget {
public:
    enum class Kind { Display, Sliders };
    explicit WelcomeGlyph(Kind kind,QWidget *parent=nullptr):QWidget(parent),m_kind(kind){setFixedSize(64,64);setAttribute(Qt::WA_TranslucentBackground);}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);painter.setPen(QPen(QColor(QStringLiteral("#808080")),1));painter.setBrush(Qt::white);painter.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),8,8);
        QPen ink(QColor(QStringLiteral("#202020")),2.2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);painter.setPen(ink);painter.setBrush(Qt::NoBrush);
        if(m_kind==Kind::Display){painter.drawRoundedRect(QRectF(16,16,32,22),3,3);painter.drawLine(QPointF(32,38),QPointF(32,45));painter.drawLine(QPointF(24,45),QPointF(40,45));}
        else {for(const qreal y:{19.0,32.0,45.0})painter.drawLine(QPointF(16,y),QPointF(48,y));painter.setBrush(Qt::white);painter.drawEllipse(QPointF(27,19),3.5,3.5);painter.drawEllipse(QPointF(39,32),3.5,3.5);painter.drawEllipse(QPointF(23,45),3.5,3.5);}
    }
private:
    Kind m_kind;
};

class OverflowArrowIcon final : public QWidget {
public:
    explicit OverflowArrowIcon(QWidget *parent=nullptr):QWidget(parent){setObjectName(QStringLiteral("welcome-overflow-arrow-icon"));setFixedSize(18,18);setAttribute(Qt::WA_TranslucentBackground);}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);painter.setPen(QPen(QColor(QStringLiteral("#202020")),2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));painter.drawLine(QPointF(4,11),QPointF(9,6));painter.drawLine(QPointF(9,6),QPointF(14,11));
    }
};

} // namespace

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
    auto *root=new QVBoxLayout(this);root->setContentsMargins(28,26,28,26);root->setSpacing(16);
    auto *titleRow=new QHBoxLayout;
    auto *icon=new QLabel;icon->setPixmap(QIcon(QStringLiteral(":/app-icon.png")).pixmap(64,64));titleRow->addWidget(icon);
    auto *title=new SmoothLabel(L("welcome.title"));QFont titleFont=title->font();titleFont.setPointSize(19);titleFont.setWeight(QFont::DemiBold);title->setFont(titleFont);title->setWordWrap(true);titleRow->addWidget(title,1);root->addLayout(titleRow);
    auto *tipsCard=new QFrame;tipsCard->setObjectName(QStringLiteral("welcome-tray-guide"));tipsCard->setStyleSheet(QStringLiteral("QFrame#welcome-tray-guide{border:2px solid #202020;border-radius:7px;background:white;}QFrame#welcome-tray-guide QLabel{border:0;background:transparent;}"));
    auto *tipsLayout=new QVBoxLayout(tipsCard);tipsLayout->setContentsMargins(14,12,14,12);tipsLayout->setSpacing(12);
    auto addTip=[&](QWidget *tipIcon,const QString &heading,const QString &body,const QString &bodyObjectName,bool showOverflowArrow){
        auto *row=new QHBoxLayout;row->setSpacing(12);row->setAlignment(Qt::AlignTop);row->addWidget(tipIcon,0,Qt::AlignTop);auto *text=new QWidget;auto *textLayout=new QVBoxLayout(text);textLayout->setContentsMargins(0,0,0,0);textLayout->setSpacing(2);auto *head=new SmoothLabel(heading);QFont f=head->font();f.setWeight(QFont::DemiBold);head->setFont(f);textLayout->addWidget(head);QString plainBody=body;plainBody.remove(QStringLiteral("**"));auto *description=new SmoothLabel(plainBody);if(!bodyObjectName.isEmpty())description->setObjectName(bodyObjectName);description->setWordWrap(true);description->setStyleSheet(QStringLiteral("color:#4a4a4a;border:0;"));if(showOverflowArrow){auto *bodyRow=new QHBoxLayout;bodyRow->setContentsMargins(0,0,0,0);bodyRow->setSpacing(4);bodyRow->addWidget(new OverflowArrowIcon,0,Qt::AlignTop);bodyRow->addWidget(description,1);textLayout->addLayout(bodyRow);}else textLayout->addWidget(description);row->addWidget(text,1);tipsLayout->addLayout(row);
    };
    auto *trayIcon=new QLabel;trayIcon->setObjectName(QStringLiteral("welcome-tray-icon"));trayIcon->setAlignment(Qt::AlignCenter);trayIcon->setFixedSize(64,64);QPixmap trayPreview=QPixmap::fromImage(ui::bookPagesTrayImage(192,true));trayPreview.setDevicePixelRatio(4.0);trayIcon->setPixmap(trayPreview);trayIcon->setStyleSheet(QStringLiteral("QLabel{background:white;border:1px solid #808080;border-radius:8px;}"));
    addTip(trayIcon,L("welcome.windows.tray.title"),L("welcome.windows.tray"),QStringLiteral("welcome-tray-instructions"),true);
    addTip(new WelcomeGlyph(WelcomeGlyph::Kind::Display),L("welcome.bigme.title"),L("welcome.bigme.body"),QString(),false);
    addTip(new WelcomeGlyph(WelcomeGlyph::Kind::Sliders),L("welcome.other.title"),L("welcome.other.body"),QString(),false);
    root->addWidget(tipsCard);
    auto *bottom=new QHBoxLayout;auto *hide=new QCheckBox(L("welcome.hide"));hide->setObjectName(QStringLiteral("welcome-hide"));bottom->addWidget(hide);bottom->addStretch();
    auto *done=ui::outlinedButton(L("welcome.done"),QStringLiteral("welcome-done"));bottom->addWidget(done);root->addLayout(bottom);
    connect(done,&QPushButton::clicked,this,[this,hide]{if(hide->isChecked())m_controller->setShowWelcome(false);accept();});
}

void WelcomeDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);adjustSize();positionNearNotificationArea();raise();
    const QPointer<WelcomeDialog> self(this);QTimer::singleShot(80,this,[self]{if(self&&self->isVisible())self->positionNearNotificationArea();});
}

void WelcomeDialog::positionNearNotificationArea() {
    QScreen *screen=QGuiApplication::primaryScreen();
    if(!screen)screen=QGuiApplication::primaryScreen();if(!screen)return;
    if(!windowHandle())winId();if(QWindow *window=windowHandle();window&&window->screen()!=screen)window->setScreen(screen);
    const QRect available=screen->availableGeometry();constexpr int margin=16;
    move(qMax(available.left()+margin,available.right()-width()-margin+1),qMax(available.top()+margin,available.bottom()-height()-margin+1));
    update();
}

void WelcomeDialog::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    constexpr qreal radius=11,pad=3;
    const QRectF body(pad,pad,width()-2*pad,height()-2*pad);
    QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setPen(QPen(QColor(QStringLiteral("#202020")),2));painter.setBrush(Qt::white);painter.drawRoundedRect(body,radius,radius);
}

} // namespace eink
