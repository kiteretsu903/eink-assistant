#pragma once

#include <QLabel>

namespace eink {

// QLabel's opaque backing-store path uses LCD glyph masks. Chrome-like text is
// composited from a transparent glyph layer, producing stable grayscale edges.
class SmoothLabel final : public QLabel {
    Q_OBJECT
public:
    explicit SmoothLabel(QWidget *parent=nullptr);
    explicit SmoothLabel(const QString &text,QWidget *parent=nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
};

} // namespace eink
