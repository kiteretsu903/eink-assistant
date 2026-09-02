#pragma once

#include "app/ApplicationController.h"

#include <QWidget>
#include <QSet>

class QLabel;
class QScrollArea;
class QVBoxLayout;
class QEvent;
class QPaintEvent;
class QScreen;

namespace eink {

class MainPanel : public QWidget {
    Q_OBJECT
public:
    explicit MainPanel(ApplicationController *controller, QWidget *parent=nullptr);
    void showPanel(QScreen *preferredScreen = nullptr);
    void beginShutdown();
    void setConfigurationBusy(bool busy, const QString &message = {}, int delayMs = 0);

signals:
    void quitRequested();

protected:
    void closeEvent(QCloseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void scheduleFocusLossHide();
    void rebuildContent();
    void placeOnScreen(QScreen *screen);
    QWidget *systemVisualEffectsSection();
    QWidget *nightLightControlSection();
    QWidget *nightLightFallbackSection();
    QWidget *windowsLightModeSection();
    QWidget *helpSection();
    QLabel *makeLabel(const QString &text,bool heading=false,bool secondary=false);

    ApplicationController *m_controller;
    QScrollArea *m_scroll;
    QWidget *m_content;
    QVBoxLayout *m_contentLayout;
    QLabel *m_error;
    QWidget *m_processingIndicator;
    QLabel *m_processingMessage;
    bool m_helpExpanded=false;
    bool m_rebuildPending=false;
    bool m_configurationBusy=false;
    bool m_nightLightTransitionActive=false;
    bool m_shuttingDown=false;
    quint64 m_busyGeneration=0;
    quint64 m_placementGeneration=0;
    QSet<QString> m_rgbExpandedDisplays;
};

} // namespace eink
