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
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QPointer>
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
    auto *title=makeLabel(L("app.title"),true);title->setObjectName(QStringLiteral("header-title"));QFont titleFont=title->font();titleFont.setPointSize(17);title->setFont(titleFont);headerLayout->addWidget(title);auto *version=makeLabel(QStringLiteral("v2.3 Windows"),false,true);version->setWordWrap(false);headerLayout->addWidget(version);headerLayout->addStretch();
    m_processingIndicator=new QWidget(header);m_processingIndicator->setObjectName(QStringLiteral("inline-processing"));m_processingIndicator->setStyleSheet(QStringLiteral("QWidget#inline-processing{background:#f4f4f4;border:1px solid #777777;border-radius:6px;}QWidget#inline-processing-spinner{background:transparent;border:0;}QWidget#inline-processing QLabel{background:transparent;border:0;font-size:12px;font-weight:600;}"));auto *processingLayout=new QHBoxLayout(m_processingIndicator);processingLayout->setContentsMargins(7,3,8,3);processingLayout->setSpacing(6);auto *spinner=new InlineSpinner(m_processingIndicator);processingLayout->addWidget(spinner);m_processingMessage=new QLabel(L("busy.configuring"),m_processingIndicator);m_processingMessage->setObjectName(QStringLiteral("inline-processing-message"));m_processingMessage->setWordWrap(false);processingLayout->addWidget(m_processingMessage);m_processingIndicator->hide();headerLayout->addWidget(m_processingIndicator);
    auto *minimize=ui::outlinedButton(QStringLiteral("−"),QStringLiteral("minimize-button"));minimize->setFixedWidth(42);minimize->setFocusPolicy(Qt::NoFocus);minimize->setToolTip(L("minimize"));minimize->setAccessibleName(L("minimize"));connect(minimize,&QPushButton::clicked,this,&QWidget::hide);headerLayout->addWidget(minimize);auto *quit=ui::outlinedButton(L("quit"),QStringLiteral("quit-button"));connect(quit,&QPushButton::clicked,this,&MainPanel::quitRequested);headerLayout->addWidget(quit);root->addWidget(header);root->addWidget(ui::divider());
    m_scroll=new QScrollArea;m_scroll->setWidgetResizable(true);m_scroll->setMinimumHeight(0);m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);m_content=new QWidget;m_contentLayout=new QVBoxLayout(m_content);m_contentLayout->setSizeConstraint(QLayout::SetMinimumSize);m_contentLayout->setContentsMargins(18,16,18,18);m_contentLayout->setSpacing(16);m_scroll->setWidget(m_content);root->addWidget(m_scroll);
    connect(m_controller,&ApplicationController::displaysChanged,this,[this]{if(m_rebuildPending)return;m_rebuildPending=true;QTimer::singleShot(0,this,[this]{m_rebuildPending=false;rebuildContent();});});
    connect(m_controller,&ApplicationController::errorChanged,this,[this](const QString &message){if(m_error){m_error->setText(message);m_error->show();}});
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
    const bool initiallyDisabled=!m_controller->platform().nightLightEnabled();auto *row=new QHBoxLayout;row->addWidget(makeLabel(L("night.disable.title"),true));auto *status=makeLabel(initiallyDisabled?L("system.on"):L("system.off"),false,true);status->setObjectName(QStringLiteral("disable-night-light-status"));row->addWidget(status);row->addStretch();auto *slot=new QWidget;slot->setFixedSize(58,32);auto *toggle=new EinkSwitch(slot);toggle->setObjectName(QStringLiteral("disable-night-light-switch"));toggle->setChecked(initiallyDisabled);toggle->move(0,0);auto *spinner=new InlineSpinner(slot);spinner->setObjectName(QStringLiteral("disable-night-light-spinner"));spinner->setProperty("preserve-hidden",true);spinner->move((slot->width()-spinner->width())/2,(slot->height()-spinner->height())/2);spinner->hide();connect(toggle,&QAbstractButton::toggled,this,[this,toggle,spinner](bool on){toggle->hide();spinner->setRunning(true);spinner->show();m_controller->setNightLightDisabled(on);});row->addWidget(slot);v->addLayout(row);
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

void MainPanel::rebuildContent() {
    const int priorScroll=m_scroll->verticalScrollBar()->value();
    if(auto *title=findChild<QLabel*>(QStringLiteral("header-title"))){title->setText(L("app.title"));QFont font=title->font();font.setFamily(QApplication::font().family());title->setFont(font);}
    if(auto *minimize=findChild<QPushButton*>(QStringLiteral("minimize-button"))){minimize->setToolTip(L("minimize"));minimize->setAccessibleName(L("minimize"));}
    if(auto *quit=findChild<QPushButton*>(QStringLiteral("quit-button")))quit->setText(L("quit"));
    ui::clearLayout(m_contentLayout);m_contentLayout->addWidget(systemVisualEffectsSection());m_contentLayout->addWidget(ui::divider());if(m_controller->platform().nightLightControlAvailable()){m_contentLayout->addWidget(nightLightControlSection());m_contentLayout->addWidget(ui::divider());}else if(m_controller->platform().nightLightAvailable()){m_contentLayout->addWidget(nightLightFallbackSection());m_contentLayout->addWidget(ui::divider());}if(m_controller->platform().windowsLightModeAvailable()){m_contentLayout->addWidget(windowsLightModeSection());m_contentLayout->addWidget(ui::divider());}m_contentLayout->addWidget(makeLabel(L("display.mark"),false,true));
    if(m_controller->displays().isEmpty())m_contentLayout->addWidget(makeLabel(L("display.none"),false,true));
    else for(const DisplayInfo &display:m_controller->displays()) {
        auto *card=new DisplayCard(m_controller,display,m_rgbExpandedDisplays.contains(display.stableId));
        connect(card,&DisplayCard::rgbExpansionChanged,this,[this](const QString &id,bool expanded){if(expanded)m_rgbExpandedDisplays.insert(id);else m_rgbExpandedDisplays.remove(id);});
        connect(card,&DisplayCard::contentSizeChanged,this,[this]{QTimer::singleShot(0,this,[this]{if(!isVisible())return;m_contentLayout->invalidate();m_contentLayout->activate();QScreen *screen=windowHandle()?windowHandle()->screen():QGuiApplication::screenAt(QCursor::pos());if(!screen)screen=QGuiApplication::primaryScreen();placeOnScreen(screen);});});
        m_contentLayout->addWidget(card);
    }
    m_error=makeLabel(m_controller->lastError(),false,false);m_error->setObjectName(QStringLiteral("error-message"));m_error->setStyleSheet(QStringLiteral("color:#b00020;font-weight:600;"));m_error->setVisible(!m_controller->lastError().isEmpty());m_contentLayout->addWidget(m_error);m_contentLayout->addWidget(helpSection());
    auto *notice=new QHBoxLayout;notice->addWidget(makeLabel(QStringLiteral("ⓘ"),true));auto *noticeText=new QWidget;auto *noticeV=new QVBoxLayout(noticeText);noticeV->setContentsMargins(0,0,0,0);noticeV->setSpacing(3);noticeV->addWidget(makeLabel(L("notice.tuned"),false,true));noticeV->addWidget(makeLabel(L("notice.risk"),false,true));notice->addWidget(noticeText,1);m_contentLayout->addLayout(notice);m_contentLayout->addWidget(ui::divider());
    auto *languageRow=new QHBoxLayout;languageRow->addWidget(makeLabel(L("language.title")));languageRow->addStretch();auto *languages=new QComboBox;languages->setObjectName(QStringLiteral("language-combo"));languages->addItem(L("language.system"),QStringLiteral("system"));languages->addItem(QStringLiteral("English"),QStringLiteral("en"));languages->addItem(QStringLiteral("简体中文"),QStringLiteral("zh-Hans"));languages->addItem(QStringLiteral("繁體中文"),QStringLiteral("zh-Hant"));languages->addItem(QStringLiteral("日本語"),QStringLiteral("ja"));int current=languages->findData(m_controller->settings().language);languages->setCurrentIndex(current<0?0:current);connect(languages,qOverload<int>(&QComboBox::activated),this,[this,languages](int index){const QString language=languages->itemData(index).toString();Localization::instance().setLanguage(language);m_controller->setLanguage(language);QTimer::singleShot(0,this,&MainPanel::rebuildContent);});languageRow->addWidget(languages);m_contentLayout->addLayout(languageRow);
    auto *loginRow=new QHBoxLayout;loginRow->addWidget(makeLabel(L("login.toggle")));loginRow->addStretch();auto *login=new EinkSwitch;login->setObjectName(QStringLiteral("launch-login-switch"));login->setChecked(m_controller->settings().launchAtLogin);connect(login,&QAbstractButton::toggled,this,[this](bool on){m_controller->setLaunchAtLogin(on);});loginRow->addWidget(login);m_contentLayout->addLayout(loginRow);m_contentLayout->addStretch();
    if(isVisible()) {
        for(QWidget *child:m_content->findChildren<QWidget*>()) if(child!=m_error&&!child->property("preserve-hidden").toBool()) child->show();
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
    QScreen *screen=preferredScreen?preferredScreen:QGuiApplication::screenAt(QCursor::pos());
    if(!screen)screen=QGuiApplication::primaryScreen();
    const QPointer<QScreen> target(screen);const quint64 placement=++m_placementGeneration;
    placeOnScreen(screen);
    show();placeOnScreen(screen);raise();activateWindow();
    const auto settle=[this,target,placement]{if(isVisible()&&target&&placement==m_placementGeneration)placeOnScreen(target);};
    QTimer::singleShot(0,this,settle);
    QTimer::singleShot(80,this,settle);
}

void MainPanel::closeEvent(QCloseEvent *event) {hide();event->ignore();}

void MainPanel::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setPen(QPen(QColor(QStringLiteral("#202020")),2));painter.setBrush(Qt::white);
    painter.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),10,10);
}

bool MainPanel::event(QEvent *event) {
    if(event->type()==QEvent::WindowDeactivate && !m_configurationBusy) {
        QTimer::singleShot(50,this,[this]{
            if(isVisible() && !m_configurationBusy && !QApplication::activePopupWidget())hide();
        });
    }
    return QWidget::event(event);
}

} // namespace eink
