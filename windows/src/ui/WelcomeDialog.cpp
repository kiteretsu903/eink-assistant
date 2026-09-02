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
#include <QPushButton>
#include <QPointer>
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
    auto *root=new QVBoxLayout(this);root->setContentsMargins(28,26,28,26);root->setSpacing(16);
    auto *titleRow=new QHBoxLayout;
    auto *icon=new QLabel;icon->setPixmap(QIcon(QStringLiteral(":/app-icon.png")).pixmap(64,64));titleRow->addWidget(icon);
    auto *title=new SmoothLabel(L("welcome.title"));QFont titleFont=title->font();titleFont.setPointSize(19);titleFont.setWeight(QFont::DemiBold);title->setFont(titleFont);title->setWordWrap(true);titleRow->addWidget(title,1);root->addLayout(titleRow);
    auto addTip=[&](const QString &heading,const QString &body){
        auto *card=new QFrame;card->setStyleSheet(QStringLiteral("QFrame{border:2px solid #202020;border-radius:7px;background:white;}QLabel{border:0;}"));
        auto *layout=new QVBoxLayout(card);auto *head=new SmoothLabel(heading);QFont f=head->font();f.setWeight(QFont::DemiBold);head->setFont(f);layout->addWidget(head);
        QString plainBody=body;plainBody.remove(QStringLiteral("**"));
        auto *text=new SmoothLabel(plainBody);text->setWordWrap(true);text->setStyleSheet(QStringLiteral("color:#4a4a4a;border:0;"));layout->addWidget(text);root->addWidget(card);
    };
    auto *trayCard=new QFrame;trayCard->setObjectName(QStringLiteral("welcome-tray-guide"));trayCard->setStyleSheet(QStringLiteral("QFrame{border:2px solid #202020;border-radius:7px;background:#f5f5f5;}QLabel{border:0;background:transparent;}"));
    auto *trayLayout=new QHBoxLayout(trayCard);trayLayout->setContentsMargins(14,12,14,12);trayLayout->setSpacing(14);
    auto *trayIcon=new QLabel;trayIcon->setObjectName(QStringLiteral("welcome-tray-icon"));trayIcon->setAlignment(Qt::AlignCenter);trayIcon->setFixedSize(64,64);trayIcon->setPixmap(ui::bookPagesTrayIcon(true).pixmap(48,48));trayIcon->setStyleSheet(QStringLiteral("QLabel{background:white;border:1px solid #808080;border-radius:8px;}"));trayLayout->addWidget(trayIcon);
    auto *trayText=new QWidget;auto *trayTextLayout=new QVBoxLayout(trayText);trayTextLayout->setContentsMargins(0,0,0,0);trayTextLayout->setSpacing(5);auto *trayHeading=new SmoothLabel(L("welcome.windows.tray.title"));QFont trayHeadingFont=trayHeading->font();trayHeadingFont.setWeight(QFont::DemiBold);trayHeading->setFont(trayHeadingFont);trayTextLayout->addWidget(trayHeading);auto *trayBody=new SmoothLabel(L("welcome.windows.tray"));trayBody->setObjectName(QStringLiteral("welcome-tray-instructions"));trayBody->setWordWrap(true);trayBody->setStyleSheet(QStringLiteral("color:#333333;border:0;"));trayTextLayout->addWidget(trayBody);trayLayout->addWidget(trayText,1);root->addWidget(trayCard);
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
