#pragma once

#include <QDialog>

class QLabel;
class QPaintEvent;

namespace eink {

class BusyDialog final : public QDialog {
    Q_OBJECT
public:
    explicit BusyDialog(QWidget *parent=nullptr);
    void setMessage(const QString &message);
    void showCentered();
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
private:
    QWidget *m_spinner;
    QLabel *m_message;
};

} // namespace eink
