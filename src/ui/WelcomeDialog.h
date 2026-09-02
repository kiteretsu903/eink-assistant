#pragma once

#include <QDialog>
#include <QRect>

class QPaintEvent;
class QShowEvent;

namespace eink {

class ApplicationController;

class WelcomeDialog : public QDialog {
    Q_OBJECT
public:
    explicit WelcomeDialog(ApplicationController *controller, QWidget *parent=nullptr);
    void setTrayAnchorRect(const QRect &rect);
protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
private:
    enum class ArrowEdge { Top, Bottom, Left, Right };
    void positionNearNotificationArea();
    ApplicationController *m_controller;
    QRect m_trayAnchorRect;
    ArrowEdge m_arrowEdge=ArrowEdge::Bottom;
    qreal m_arrowPosition=0;
};

} // namespace eink
