#pragma once

#include "core/ToneCurve.h"
#include <QWidget>

namespace eink {

class CurvePlot : public QWidget {
    Q_OBJECT
public:
    explicit CurvePlot(QWidget *parent=nullptr);
    void setCurve(const ToneCurve &curve) { m_curve=curve; update(); }
    QSize sizeHint() const override { return {460,96}; }
protected:
    void paintEvent(QPaintEvent *) override;
private:
    ToneCurve m_curve;
};

} // namespace eink
