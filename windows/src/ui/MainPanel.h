#pragma once

#include "app/ApplicationController.h"

#include <QWidget>
#include <QSet>

class QLabel;
class QScrollArea;
class QVBoxLayout;
class QEvent;
class QPaintEvent;
class QResizeEvent;
class QScreen;
class QPushButton;

namespace eink {

class MainPanel : public QWidget {
    Q_OBJECT
public:
    explicit MainPanel(ApplicationController *controller, QWidget *parent=nullptr);
    void showPanel(QScreen *preferredScreen = nullptr);
    void showPanelAfterTransientWindow();
    void beginShutdown();
    void setConfigurationBusy(bool busy, const QString &message = {}, int delayMs = 0);

signals:
    void quitRequested();

protected:
    void closeEvent(QCloseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void scheduleFocusLossHide();
    void rebuildContent();
    void placeOnScreen(QScreen *screen);
    void setupColorSafetyOverlay();
    void updateColorSafetyOverlay(const QString &displayId, ColorSafetyPhase phase, int secondsRemaining);
    QWidget *systemVisualEffectsSection();
    QWidget *nightLightControlSection();
    QWidget *nightLightFallbackSection();
    QWidget *windowsLightModeSection();
    QWidget *hardwareSetupNoticeSection();
    QWidget *helpSection();
    QLabel *makeLabel(const QString &text,bool heading=false,bool secondary=false);

    ApplicationController *m_controller;
    QScrollArea *m_scroll;
    QWidget *m_content;
    QVBoxLayout *m_contentLayout;
    QLabel *m_error;
    QWidget *m_processingIndicator;
    QLabel *m_processingMessage;
    QWidget *m_colorSafetyOverlay=nullptr;
    QLabel *m_colorSafetyTitle=nullptr;
    QLabel *m_colorSafetyCountdown=nullptr;
    QLabel *m_colorSafetyMessage=nullptr;
    QWidget *m_colorSafetyActions=nullptr;
    QPushButton *m_colorSafetyRollback=nullptr;
    QPushButton *m_colorSafetyConfirm=nullptr;
    QString m_colorSafetyDisplayId;
    bool m_helpExpanded=false;
    bool m_rebuildPending=false;
    bool m_configurationBusy=false;
    bool m_nightLightTransitionActive=false;
    bool m_colorSafetyPromptActive=false;
    bool m_hardwareSetupNoticeDismissedForSession=false;
    bool m_shuttingDown=false;
    quint64 m_busyGeneration=0;
    quint64 m_placementGeneration=0;
    QSet<QString> m_rgbExpandedDisplays;
};

} // namespace eink
