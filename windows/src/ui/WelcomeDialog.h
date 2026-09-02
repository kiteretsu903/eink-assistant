#pragma once

#include <QDialog>
class QPaintEvent;
class QShowEvent;

namespace eink {

class ApplicationController;

class WelcomeDialog : public QDialog {
    Q_OBJECT
public:
    explicit WelcomeDialog(ApplicationController *controller, QWidget *parent=nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
private:
    void positionNearNotificationArea();
    ApplicationController *m_controller;
};

} // namespace eink
