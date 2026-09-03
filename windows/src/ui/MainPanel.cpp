#include "MainPanel.h"
#include "DisplayCard.h"
#include "EinkSwitch.h"
#include "Localization.h"
#include "SmoothLabel.h"
#include "UiStyle.h"

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCursor>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QPointer>
#include <QResizeEvent>
#include <QScreen>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

namespace eink {
namespace {

constexpr int kPanelWidth=680;

QString markdownEmphasisHtml(const QString &text) {
    const QString escaped=text.toHtmlEscaped();QString result;int cursor=0;
    while(cursor<escaped.size()) {
        const int open=escaped.indexOf(QStringLiteral("**"),cursor);
        if(open<0){result+=escaped.mid(cursor);break;}
        const int close=escaped.indexOf(QStringLiteral("**"),open+2);
        if(close<0){result+=escaped.mid(cursor);break;}
        result+=escaped.mid(cursor,open-cursor)+QStringLiteral("<b>")+escaped.mid(open+2,close-open-2)+QStringLiteral("</b>");
        cursor=close+2;
    }
    return result;
}

class HardwareNoticeIcon final : public QWidget {
public:
    explicit HardwareNoticeIcon(QWidget *parent=nullptr):QWidget(parent){setFixedSize(28,28);setAttribute(Qt::WA_TranslucentBackground);}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);painter.setPen(QPen(QColor(QStringLiteral("#147ee5")),2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(3,4,22,15),2,2);painter.drawLine(QPointF(14,19),QPointF(14,23));painter.drawLine(QPointF(9,23),QPointF(19,23));
    }
};

class InlineSpinner final : public QWidget {
public:
    explicit InlineSpinner(QWidget *parent=nullptr):QWidget(parent) {
        setObjectName(QStringLiteral("inline-processing-spinner"));setAttribute(Qt::WA_TranslucentBackground);setAutoFillBackground(false);setFixedSize(25,25);setProperty("angle",0);
        m_timer.setInterval(55);connect(&m_timer,&QTimer::timeout,this,[this]{m_step=(m_step+1)%12;setProperty("angle",m_step*30);update();});
    }
    void setRunning(bool running){if(running)m_timer.start();else m_timer.stop();update();}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);painter.translate(width()/2.0,height()/2.0);
        QPen pen;pen.setWidthF(2.2);pen.setCapStyle(Qt::RoundCap);
        for(int i=0;i<12;++i){const int distance=(i-m_step+12)%12;const int gray=qBound(20,25+distance*19,225);pen.setColor(QColor(gray,gray,gray));painter.setPen(pen);painter.drawLine(QPointF(0,-6),QPointF(0,-10));painter.rotate(30);}
    }
private:
    QTimer m_timer;
    int m_step=0;
};

} // namespace

MainPanel::MainPanel(ApplicationController *controller,QWidget *parent):QWidget(parent),m_controller(controller) {
    setObjectName(QStringLiteral("main-panel")); setWindowTitle(L("app.title")); setWindowIcon(QIcon(QStringLiteral(":/app-icon.png")));
    setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);setAttribute(Qt::WA_TranslucentBackground);setAutoFillBackground(false);setMinimumWidth(660);resize(kPanelWidth,720);setStyleSheet(ui::styleSheet()+QStringLiteral("QWidget#main-panel{border:0;background:transparent;}"));
    auto *root=new QVBoxLayout(this);root->setContentsMargins(9,9,9,9);root->setSpacing(0);
    auto *header=new QWidget;header->setFixedHeight(59);auto *headerLayout=new QHBoxLayout(header);headerLayout->setContentsMargins(18,10,18,10);
    auto *title=makeLabel(L("app.title"),true);title->setObjectName(QStringLiteral("header-title"));QFont titleFont=title->font();titleFont.setPointSize(17);title->setFont(titleFont);headerLayout->addWidget(title);auto *version=makeLabel(QStringLiteral("v1.1 Windows"),false,true);version->setObjectName(QStringLiteral("header-version"));version->setWordWrap(false);version->setMinimumWidth(version->fontMetrics().horizontalAdvance(version->text())+2);headerLayout->addWidget(version);headerLayout->addStretch();
    m_processingIndicator=new QWidget(header);m_processingIndicator->setObjectName(QStringLiteral("inline-processing"));m_processingIndicator->setStyleSheet(QStringLiteral("QWidget#inline-processing{background:#f4f4f4;border:1px solid #777777;border-radius:6px;}QWidget#inline-processing-spinner{background:transparent;border:0;}QWidget#inline-processing QLabel{background:transparent;border:0;font-size:12px;font-weight:600;}"));auto *processingLayout=new QHBoxLayout(m_processingIndicator);processingLayout->setContentsMargins(7,3,8,3);processingLayout->setSpacing(6);auto *spinner=new InlineSpinner(m_processingIndicator);processingLayout->addWidget(spinner);m_processingMessage=new QLabel(L("busy.configuring"),m_processingIndicator);m_processingMessage->setObjectName(QStringLiteral("inline-processing-message"));m_processingMessage->setWordWrap(false);processingLayout->addWidget(m_processingMessage);m_processingIndicator->hide();headerLayout->addWidget(m_processingIndicator);
    auto *minimize=ui::outlinedButton(QStringLiteral("−"),QStringLiteral("minimize-button"));minimize->setFixedWidth(42);minimize->setFocusPolicy(Qt::NoFocus);minimize->setToolTip(L("minimize"));minimize->setAccessibleName(L("minimize"));connect(minimize,&QPushButton::clicked,this,&QWidget::hide);headerLayout->addWidget(minimize);auto *quit=ui::outlinedButton(L("quit"),QStringLiteral("quit-button"));connect(quit,&QPushButton::clicked,this,&MainPanel::quitRequested);headerLayout->addWidget(quit);root->addWidget(header);root->addWidget(ui::divider());
    m_scroll=new QScrollArea;m_scroll->setWidgetResizable(true);m_scroll->setMinimumHeight(0);m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);m_content=new QWidget;m_contentLayout=new QVBoxLayout(m_content);m_contentLayout->setSizeConstraint(QLayout::SetMinimumSize);m_contentLayout->setContentsMargins(18,16,18,18);m_contentLayout->setSpacing(16);m_scroll->setWidget(m_content);root->addWidget(m_scroll);setupColorSafetyOverlay();
    connect(m_controller,&ApplicationController::displaysChanged,this,[this]{if(m_rebuildPending)return;m_rebuildPending=true;QTimer::singleShot(0,this,[this]{m_rebuildPending=false;rebuildContent();});});
    connect(m_controller,&ApplicationController::errorChanged,this,[this](const QString &message){if(m_error){m_error->setText(message);m_error->show();}});
    connect(m_controller,&ApplicationController::nightLightOperationFinished,this,[this]{m_nightLightTransitionActive=false;if(!isVisible())return;QTimer::singleShot(0,this,[this]{if(isVisible()){raise();activateWindow();}});});
    connect(m_controller,&ApplicationController::colorSafetyStateChanged,this,&MainPanel::updateColorSafetyOverlay);
    qApp->installEventFilter(this);
    Localization::instance().setLanguage(m_controller->settings().language); rebuildContent();
}

QLabel *MainPanel::makeLabel(const QString &text,bool heading,bool secondary) {auto *l=new SmoothLabel(text);l->setWordWrap(true);if(heading)l->setProperty("heading",true);if(secondary)l->setProperty("secondary",true);return l;}

void MainPanel::setConfigurationBusy(bool busy,const QString &message,int delayMs) {
    const quint64 generation=++m_busyGeneration;
    m_configurationBusy=busy;
    if(!message.isEmpty())m_processingMessage->setText(message);
    else if(busy)m_processingMessage->setText(L("busy.configuring"));
    auto *spinner=static_cast<InlineSpinner*>(m_processingIndicator->findChild<QWidget*>(QStringLiteral("inline-processing-spinner")));if(spinner)spinner->setRunning(busy);
    if(!busy){m_processingIndicator->hide();if(spinner)spinner->setRunning(false);unsetCursor();return;}
    setCursor(Qt::ArrowCursor);
    if(delayMs<=0){m_processingIndicator->show();return;}
    m_processingIndicator->hide();if(spinner)spinner->setRunning(false);
    QTimer::singleShot(delayMs,this,[this,generation]{if(!m_configurationBusy||generation!=m_busyGeneration)return;auto *pendingSpinner=static_cast<InlineSpinner*>(m_processingIndicator->findChild<QWidget*>(QStringLiteral("inline-processing-spinner")));if(pendingSpinner)pendingSpinner->setRunning(true);m_processingIndicator->show();});
}

QWidget *MainPanel::systemVisualEffectsSection() {
    auto *w=new QWidget;auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(7);auto *row=new QHBoxLayout;auto *systemTitle=makeLabel(L("system.title"),true);systemTitle->setWordWrap(false);systemTitle->setMinimumWidth(systemTitle->fontMetrics().horizontalAdvance(systemTitle->text())+2);row->addWidget(systemTitle);auto *status=makeLabel(m_controller->platform().visualEffectsReduced()?L("system.on"):L("system.off"),false,true);status->setObjectName(QStringLiteral("visual-effects-status"));row->addWidget(status);row->addStretch();auto *slot=new QWidget;slot->setFixedSize(58,32);auto *toggle=new EinkSwitch(slot);toggle->setObjectName(QStringLiteral("visual-effects-switch"));toggle->setChecked(m_controller->platform().visualEffectsReduced());toggle->move(0,0);auto *spinner=new InlineSpinner(slot);spinner->setObjectName(QStringLiteral("visual-effects-spinner"));spinner->setProperty("preserve-hidden",true);spinner->move((slot->width()-spinner->width())/2,(slot->height()-spinner->height())/2);spinner->hide();connect(toggle,&QAbstractButton::toggled,this,[this,toggle,spinner](bool on){toggle->hide();spinner->setRunning(true);spinner->show();m_controller->setVisualEffects(on);});connect(m_controller,&ApplicationController::visualEffectsStateChanged,toggle,[toggle,spinner,status](bool reduced){spinner->setRunning(false);spinner->hide();QSignalBlocker blocker(toggle);toggle->setChecked(reduced);toggle->show();status->setText(reduced?L("system.on"):L("system.off"));});row->addWidget(slot);v->addLayout(row);
    auto *autoRow=new QHBoxLayout;auto *autoLabel=makeLabel(L("system.auto"));autoLabel->setObjectName(QStringLiteral("auto-visual-effects-label"));autoLabel->setWordWrap(false);autoLabel->setMinimumWidth(autoLabel->fontMetrics().horizontalAdvance(autoLabel->text())+2);autoRow->addWidget(autoLabel);autoRow->addStretch();auto *autoToggle=new EinkSwitch;autoToggle->setObjectName(QStringLiteral("auto-visual-effects-switch"));autoToggle->setChecked(m_controller->settings().autoVisualEffects);connect(autoToggle,&QAbstractButton::toggled,this,[this](bool on){m_controller->setAutoVisualEffects(on);});autoRow->addWidget(autoToggle);v->addLayout(autoRow);return w;
}

QWidget *MainPanel::windowsLightModeSection() {
    auto *w=new QWidget;auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(7);
    auto *row=new QHBoxLayout;row->addWidget(makeLabel(L("system.lightMode"),true));auto *status=makeLabel(m_controller->platform().windowsLightModeEnabled()?L("system.on"):L("system.off"),false,true);status->setObjectName(QStringLiteral("windows-light-mode-status"));row->addWidget(status);row->addStretch();auto *slot=new QWidget;slot->setFixedSize(58,32);auto *toggle=new EinkSwitch(slot);toggle->setObjectName(QStringLiteral("windows-light-mode-switch"));toggle->setChecked(m_controller->platform().windowsLightModeEnabled());toggle->setEnabled(m_controller->platform().windowsLightModeAvailable());toggle->move(0,0);auto *spinner=new InlineSpinner(slot);spinner->setObjectName(QStringLiteral("windows-light-mode-spinner"));spinner->setProperty("preserve-hidden",true);spinner->move((slot->width()-spinner->width())/2,(slot->height()-spinner->height())/2);spinner->hide();connect(toggle,&QAbstractButton::toggled,this,[this,toggle,spinner](bool on){toggle->hide();spinner->setRunning(true);spinner->show();m_controller->setWindowsLightMode(on);});connect(m_controller,&ApplicationController::windowsLightModeStateChanged,toggle,[toggle,spinner,status](bool enabled){spinner->setRunning(false);spinner->hide();QSignalBlocker blocker(toggle);toggle->setChecked(enabled);toggle->show();status->setText(enabled?L("system.on"):L("system.off"));});row->addWidget(slot);v->addLayout(row);
    auto *note=makeLabel(m_controller->platform().windowsLightModeAvailable()?L("system.lightMode.note"):L("system.lightMode.unavailable"),false,true);note->setObjectName(QStringLiteral("windows-light-mode-note"));note->setWordWrap(false);v->addWidget(note);
    return w;
}

QWidget *MainPanel::nightLightControlSection() {
    auto *w=new QWidget;auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(7);
    const bool initiallyDisabled=!m_controller->platform().nightLightEnabled();auto *row=new QHBoxLayout;row->addWidget(makeLabel(L("night.disable.title"),true));auto *status=makeLabel(initiallyDisabled?L("system.on"):L("system.off"),false,true);status->setObjectName(QStringLiteral("disable-night-light-status"));row->addWidget(status);row->addStretch();auto *slot=new QWidget;slot->setFixedSize(58,32);auto *toggle=new EinkSwitch(slot);toggle->setObjectName(QStringLiteral("disable-night-light-switch"));toggle->setChecked(initiallyDisabled);toggle->move(0,0);auto *spinner=new InlineSpinner(slot);spinner->setObjectName(QStringLiteral("disable-night-light-spinner"));spinner->setProperty("preserve-hidden",true);spinner->move((slot->width()-spinner->width())/2,(slot->height()-spinner->height())/2);spinner->hide();connect(toggle,&QAbstractButton::toggled,this,[this,toggle,spinner](bool on){m_nightLightTransitionActive=true;toggle->hide();spinner->setRunning(true);spinner->show();m_controller->setNightLightDisabled(on);});row->addWidget(slot);v->addLayout(row);
    auto *note=makeLabel(QStringLiteral("⚠ ")+L("night.disable.note"),false,true);note->setObjectName(QStringLiteral("disable-night-light-note"));note->setProperty("preserve-hidden",true);note->setWordWrap(false);note->setStyleSheet(QStringLiteral("color:#a64b00;font-weight:700;"));note->setVisible(!initiallyDisabled);v->addWidget(note);
    connect(m_controller,&ApplicationController::nightLightStateChanged,toggle,[toggle,spinner,status,note](bool disabled){spinner->setRunning(false);spinner->hide();QSignalBlocker blocker(toggle);toggle->setChecked(disabled);toggle->show();status->setText(disabled?L("system.on"):L("system.off"));note->setVisible(!disabled);});return w;
}

QWidget *MainPanel::nightLightFallbackSection() {
    auto *w=new QWidget;auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(7);
    auto *row=new QHBoxLayout;row->addWidget(makeLabel(L("night.title"),true));row->addStretch();auto *open=ui::outlinedButton(L("night.open"),QStringLiteral("night-light-button"));
    connect(open,&QPushButton::clicked,this,[this]{m_controller->platform().openNightLightSettings();});row->addWidget(open);v->addLayout(row);
    auto *recommendation=makeLabel(QStringLiteral("⚠ ")+L("night.fallback.recommendation"));recommendation->setObjectName(QStringLiteral("night-light-fallback-recommendation"));recommendation->setStyleSheet(QStringLiteral("color:#a64b00;font-weight:700;"));v->addWidget(recommendation);
    auto *path=makeLabel(L("night.fallback.path"),false,true);path->setObjectName(QStringLiteral("night-light-settings-path"));v->addWidget(path);return w;
}

QWidget *MainPanel::helpSection() {
    auto *w=new QWidget;auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(7);auto *row=new QHBoxLayout;row->addWidget(makeLabel(L("help.title"),true));auto *button=ui::outlinedButton(m_helpExpanded?L("help.less"):L("help.more"),QStringLiteral("help-toggle"));connect(button,&QPushButton::clicked,this,[this]{m_helpExpanded=!m_helpExpanded;rebuildContent();});row->addWidget(button);row->addStretch();v->addLayout(row);if(m_helpExpanded)v->addWidget(makeLabel(L("help.windows"),false,true));return w;
}

void MainPanel::setupColorSafetyOverlay() {
    m_colorSafetyOverlay=new QWidget(this);m_colorSafetyOverlay->setObjectName(QStringLiteral("color-safety-overlay"));m_colorSafetyOverlay->setAttribute(Qt::WA_StyledBackground,true);m_colorSafetyOverlay->setStyleSheet(QStringLiteral("QWidget#color-safety-overlay{background:rgba(0,0,0,145);border-radius:10px;}QFrame#color-safety-dialog{background:white;border:3px solid #202020;border-radius:10px;}QFrame#color-safety-dialog QLabel{background:transparent;border:0;}"));
    auto *overlayLayout=new QVBoxLayout(m_colorSafetyOverlay);overlayLayout->setContentsMargins(30,30,30,30);overlayLayout->addStretch();auto *dialog=new QFrame;dialog->setObjectName(QStringLiteral("color-safety-dialog"));dialog->setMinimumSize(430,240);dialog->setMaximumWidth(510);auto *dialogLayout=new QVBoxLayout(dialog);dialogLayout->setContentsMargins(24,22,24,22);dialogLayout->setSpacing(14);
    m_colorSafetyTitle=makeLabel(QString(),true);m_colorSafetyTitle->setObjectName(QStringLiteral("color-safety-title"));m_colorSafetyTitle->setAlignment(Qt::AlignCenter);QFont titleFont=m_colorSafetyTitle->font();titleFont.setPointSize(19);titleFont.setWeight(QFont::Black);m_colorSafetyTitle->setFont(titleFont);dialogLayout->addWidget(m_colorSafetyTitle);
    m_colorSafetyCountdown=makeLabel(QString(),true);m_colorSafetyCountdown->setObjectName(QStringLiteral("color-safety-countdown"));m_colorSafetyCountdown->setAlignment(Qt::AlignCenter);QFont countdownFont=m_colorSafetyCountdown->font();countdownFont.setPointSize(32);countdownFont.setWeight(QFont::Black);m_colorSafetyCountdown->setFont(countdownFont);m_colorSafetyCountdown->setStyleSheet(QStringLiteral("font-size:32pt;font-weight:900;color:#202020;"));dialogLayout->addWidget(m_colorSafetyCountdown);
    m_colorSafetyMessage=makeLabel(QString(),false,true);m_colorSafetyMessage->setObjectName(QStringLiteral("color-safety-message"));m_colorSafetyMessage->setAlignment(Qt::AlignCenter);dialogLayout->addWidget(m_colorSafetyMessage);
    m_colorSafetyActions=new QWidget;m_colorSafetyActions->setObjectName(QStringLiteral("color-safety-actions"));auto *actions=new QHBoxLayout(m_colorSafetyActions);actions->setContentsMargins(0,2,0,0);actions->setSpacing(12);m_colorSafetyRollback=ui::outlinedButton(QString(),QStringLiteral("color-safety-rollback"));m_colorSafetyConfirm=ui::outlinedButton(QString(),QStringLiteral("color-safety-confirm"));m_colorSafetyRollback->setFixedHeight(42);m_colorSafetyConfirm->setFixedHeight(42);QFont confirmFont=m_colorSafetyConfirm->font();confirmFont.setWeight(QFont::Black);m_colorSafetyConfirm->setFont(confirmFont);m_colorSafetyConfirm->setStyleSheet(QStringLiteral("QPushButton{background:#c62828;color:white;border:3px solid #8e0000;font-weight:900;padding:8px 14px;}QPushButton:hover{background:#b71c1c;}QPushButton:pressed{background:#8e0000;}"));connect(m_colorSafetyRollback,&QPushButton::clicked,this,[this]{const QString displayId=m_colorSafetyDisplayId;if(!displayId.isEmpty())m_controller->rollbackExperimentalColor(displayId);});connect(m_colorSafetyConfirm,&QPushButton::clicked,this,[this]{const QString displayId=m_colorSafetyDisplayId;if(!displayId.isEmpty())m_controller->confirmExperimentalColor(displayId);});actions->addStretch();actions->addWidget(m_colorSafetyRollback);actions->addWidget(m_colorSafetyConfirm);actions->addStretch();dialogLayout->addWidget(m_colorSafetyActions);overlayLayout->addWidget(dialog,0,Qt::AlignHCenter);overlayLayout->addStretch();m_colorSafetyOverlay->setGeometry(rect());m_colorSafetyOverlay->hide();
}

void MainPanel::updateColorSafetyOverlay(const QString &displayId,ColorSafetyPhase phase,int secondsRemaining) {
    if(phase==ColorSafetyPhase::Idle){m_colorSafetyPromptActive=false;m_colorSafetyDisplayId.clear();m_colorSafetyOverlay->hide();return;}
    m_colorSafetyPromptActive=true;m_colorSafetyDisplayId=displayId;const bool confirming=phase==ColorSafetyPhase::AwaitingConfirmation;m_colorSafetyTitle->setText(L(confirming?"saturation.experimental.dialog.confirm.title":"saturation.experimental.dialog.preparing.title"));m_colorSafetyMessage->setText(L(confirming?"saturation.experimental.dialog.confirm.body":"saturation.experimental.dialog.preparing.body"));QString countdown=L("saturation.experimental.seconds");countdown.replace(QStringLiteral("%1"),QString::number(secondsRemaining));m_colorSafetyCountdown->setText(countdown);m_colorSafetyRollback->setText(L("saturation.experimental.rollback"));m_colorSafetyConfirm->setText(L("saturation.experimental.confirmButton"));m_colorSafetyActions->setVisible(confirming);m_colorSafetyOverlay->setGeometry(rect());m_colorSafetyOverlay->show();m_colorSafetyOverlay->raise();
}

QWidget *MainPanel::hardwareSetupNoticeSection() {
    auto *card=new QFrame;card->setObjectName(QStringLiteral("hardware-setup-notice"));card->setStyleSheet(QStringLiteral("QFrame#hardware-setup-notice{border:2px solid #202020;border-radius:7px;background:white;}QFrame#hardware-setup-notice QLabel{border:0;background:transparent;}"));
    auto *layout=new QVBoxLayout(card);layout->setContentsMargins(14,12,14,12);layout->setSpacing(10);
    auto *content=new QHBoxLayout;content->setSpacing(9);content->setAlignment(Qt::AlignTop);content->addWidget(new HardwareNoticeIcon,0,Qt::AlignTop);
    auto *text=new QWidget;auto *textLayout=new QVBoxLayout(text);textLayout->setContentsMargins(0,0,0,0);textLayout->setSpacing(5);auto *heading=makeLabel(L("hardware.notice.title"),true);heading->setObjectName(QStringLiteral("hardware-notice-title"));textLayout->addWidget(heading);auto *body=new QLabel(markdownEmphasisHtml(L("hardware.notice.body")));body->setTextFormat(Qt::RichText);body->setWordWrap(true);body->setTextInteractionFlags(Qt::NoTextInteraction);body->setObjectName(QStringLiteral("hardware-notice-body"));textLayout->addWidget(body);auto *example=makeLabel(L("hardware.notice.bigme"),false,true);example->setObjectName(QStringLiteral("hardware-notice-bigme"));textLayout->addWidget(example);content->addWidget(text,1);layout->addLayout(content);
    auto *actions=new QHBoxLayout;actions->addStretch();auto *gotIt=ui::outlinedButton(L("hardware.notice.gotIt"),QStringLiteral("hardware-notice-got-it"));gotIt->setStyleSheet(QStringLiteral("color:#147ee5;border-color:#147ee5;"));connect(gotIt,&QPushButton::clicked,this,[this]{m_hardwareSetupNoticeDismissedForSession=true;rebuildContent();});actions->addWidget(gotIt);auto *never=ui::outlinedButton(L("hardware.notice.never"),QStringLiteral("hardware-notice-never"));connect(never,&QPushButton::clicked,this,[this]{m_controller->setHardwareSetupNoticeHidden(true);rebuildContent();});actions->addWidget(never);layout->addLayout(actions);return card;
}

void MainPanel::rebuildContent() {
    const int priorScroll=m_scroll->verticalScrollBar()->value();
    if(auto *title=findChild<QLabel*>(QStringLiteral("header-title"))){title->setText(L("app.title"));QFont font=title->font();font.setFamily(QApplication::font().family());title->setFont(font);}
    if(auto *minimize=findChild<QPushButton*>(QStringLiteral("minimize-button"))){minimize->setToolTip(L("minimize"));minimize->setAccessibleName(L("minimize"));}
    if(auto *quit=findChild<QPushButton*>(QStringLiteral("quit-button")))quit->setText(L("quit"));
    ui::clearLayout(m_contentLayout);if(!m_hardwareSetupNoticeDismissedForSession&&!m_controller->settings().hideHardwareSetupNotice){m_contentLayout->addWidget(hardwareSetupNoticeSection());m_contentLayout->addWidget(ui::divider());}m_contentLayout->addWidget(systemVisualEffectsSection());m_contentLayout->addWidget(ui::divider());if(m_controller->platform().nightLightControlAvailable()){m_contentLayout->addWidget(nightLightControlSection());m_contentLayout->addWidget(ui::divider());}else if(m_controller->platform().nightLightAvailable()){m_contentLayout->addWidget(nightLightFallbackSection());m_contentLayout->addWidget(ui::divider());}if(m_controller->platform().windowsLightModeAvailable()){m_contentLayout->addWidget(windowsLightModeSection());m_contentLayout->addWidget(ui::divider());}m_contentLayout->addWidget(makeLabel(L("display.mark"),false,true));
    if(m_controller->displays().isEmpty())m_contentLayout->addWidget(makeLabel(L("display.none"),false,true));
    else for(const DisplayInfo &display:m_controller->displays()) {
        auto *card=new DisplayCard(m_controller,display,m_rgbExpandedDisplays.contains(display.stableId));
        connect(card,&DisplayCard::rgbExpansionChanged,this,[this](const QString &id,bool expanded){if(expanded)m_rgbExpandedDisplays.insert(id);else m_rgbExpandedDisplays.remove(id);});
        connect(card,&DisplayCard::contentSizeChanged,this,[this]{QTimer::singleShot(0,this,[this]{if(!isVisible())return;m_contentLayout->invalidate();m_contentLayout->activate();QScreen *screen=windowHandle()?windowHandle()->screen():QGuiApplication::screenAt(QCursor::pos());if(!screen)screen=QGuiApplication::primaryScreen();placeOnScreen(screen);});});
        m_contentLayout->addWidget(card);
    }
    m_error=makeLabel(m_controller->lastError(),false,false);m_error->setObjectName(QStringLiteral("error-message"));m_error->setStyleSheet(QStringLiteral("color:#b00020;font-weight:600;"));m_error->setVisible(!m_controller->lastError().isEmpty());m_contentLayout->addWidget(m_error);m_contentLayout->addWidget(helpSection());m_contentLayout->addWidget(ui::divider());
    auto *languageRow=new QHBoxLayout;languageRow->addWidget(makeLabel(L("language.title")));languageRow->addStretch();auto *languages=new QComboBox;languages->setObjectName(QStringLiteral("language-combo"));languages->addItem(L("language.system"),QStringLiteral("system"));languages->addItem(QStringLiteral("English"),QStringLiteral("en"));languages->addItem(QStringLiteral("简体中文"),QStringLiteral("zh-Hans"));languages->addItem(QStringLiteral("繁體中文"),QStringLiteral("zh-Hant"));languages->addItem(QStringLiteral("日本語"),QStringLiteral("ja"));int current=languages->findData(m_controller->settings().language);languages->setCurrentIndex(current<0?0:current);connect(languages,qOverload<int>(&QComboBox::activated),this,[this,languages](int index){const QString language=languages->itemData(index).toString();Localization::instance().setLanguage(language);m_controller->setLanguage(language);QTimer::singleShot(0,this,&MainPanel::rebuildContent);});languageRow->addWidget(languages);m_contentLayout->addLayout(languageRow);
    auto *loginRow=new QHBoxLayout;loginRow->addWidget(makeLabel(L("login.toggle")));loginRow->addStretch();auto *login=new EinkSwitch;login->setObjectName(QStringLiteral("launch-login-switch"));login->setChecked(m_controller->settings().launchAtLogin);connect(login,&QAbstractButton::toggled,this,[this](bool on){m_controller->setLaunchAtLogin(on);});loginRow->addWidget(login);m_contentLayout->addLayout(loginRow);m_contentLayout->addStretch();
    if(isVisible()) {
        for(QWidget *child:m_content->findChildren<QWidget*>()) if(child!=m_error&&!child->property("preserve-hidden").toBool()&&!child->property("_einkPendingDelete").toBool()) child->show();
        if(m_error&&!m_controller->lastError().isEmpty())m_error->show();
    }
    QTimer::singleShot(0,this,[this,priorScroll]{m_scroll->verticalScrollBar()->setValue(priorScroll);});
}

void MainPanel::placeOnScreen(QScreen *screen) {
    if(!screen)return;
    if(!windowHandle())winId();
    if(QWindow *window=windowHandle();window&&window->screen()!=screen)window->setScreen(screen);
    const QRect area=screen->availableGeometry();
    if(m_contentLayout)m_contentLayout->activate();
    const QMargins outer=layout()?layout()->contentsMargins():QMargins();
    const int chromeHeight=60+outer.top()+outer.bottom()+(m_scroll?m_scroll->frameWidth()*2:0);
    const int naturalHeight=chromeHeight+(m_contentLayout?m_contentLayout->sizeHint().height():0);
    const int height=qMin(qMax(1,area.height()-32),qMax(240,naturalHeight));
    resize(kPanelWidth,height);
    move(area.right()-width()+1-16,area.bottom()-height+1-16);
}

void MainPanel::showPanel(QScreen *preferredScreen) {
    if(m_shuttingDown)return;
    QScreen *screen=preferredScreen?preferredScreen:QGuiApplication::screenAt(QCursor::pos());
    if(!screen)screen=QGuiApplication::primaryScreen();
    const QPointer<QScreen> target(screen);const quint64 placement=++m_placementGeneration;
    placeOnScreen(screen);
    show();placeOnScreen(screen);raise();activateWindow();
    const auto settle=[this,target,placement]{if(isVisible()&&target&&placement==m_placementGeneration)placeOnScreen(target);};
    QTimer::singleShot(0,this,settle);
    QTimer::singleShot(80,this,settle);
}

void MainPanel::showPanelAfterTransientWindow() {
    QTimer::singleShot(100,this,[this]{if(!m_shuttingDown)showPanel();});
}

void MainPanel::beginShutdown() {
    if(m_shuttingDown)return;
    m_shuttingDown=true;
    ++m_placementGeneration;
    setConfigurationBusy(false);
    if(m_colorSafetyOverlay)m_colorSafetyOverlay->hide();
    hide();
}

void MainPanel::closeEvent(QCloseEvent *event) {hide();event->ignore();}

void MainPanel::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setPen(QPen(QColor(QStringLiteral("#202020")),2));painter.setBrush(Qt::white);
    painter.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),10,10);
}

void MainPanel::resizeEvent(QResizeEvent *event) {QWidget::resizeEvent(event);if(m_colorSafetyOverlay)m_colorSafetyOverlay->setGeometry(rect());}

void MainPanel::scheduleFocusLossHide() {
    if(m_shuttingDown||m_configurationBusy||m_nightLightTransitionActive||m_colorSafetyPromptActive)return;
    QTimer::singleShot(50,this,[this]{
        if(isVisible() && !m_configurationBusy && !m_nightLightTransitionActive && !m_colorSafetyPromptActive && !QApplication::activePopupWidget())hide();
    });
}

bool MainPanel::event(QEvent *event) {
    if(event->type()==QEvent::WindowDeactivate)scheduleFocusLossHide();
    return QWidget::event(event);
}

bool MainPanel::eventFilter(QObject *watched,QEvent *event) {
    if(watched==qApp && event->type()==QEvent::ApplicationDeactivate)scheduleFocusLossHide();
    return QWidget::eventFilter(watched,event);
}

} // namespace eink
