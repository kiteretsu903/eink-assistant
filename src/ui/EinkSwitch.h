#pragma once

#include <QAbstractButton>

namespace eink {

class EinkSwitch : public QAbstractButton {
    Q_OBJECT
public:
    explicit EinkSwitch(QWidget *parent=nullptr);
    QSize sizeHint() const override { return {58,32}; }
protected:
    void paintEvent(QPaintEvent *) override;
};

} // namespace eink
