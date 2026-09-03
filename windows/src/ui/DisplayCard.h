#pragma once

#include "app/ApplicationController.h"

#include <QFrame>

class QVBoxLayout;
class QLabel;

namespace eink {

class DisplayCard : public QFrame {
    Q_OBJECT
public:
    DisplayCard(ApplicationController *controller, DisplayInfo info, bool rgbExpanded=false, QWidget *parent=nullptr);
    const QString &displayId() const { return m_info.stableId; }
    void rebuild();

signals:
    void rgbExpansionChanged(const QString &displayId, bool expanded);
    void contentSizeChanged();

private:
    QLabel *label(const QString &text, bool heading=false, bool secondary=false);
    QWidget *textSelector(const DisplaySettings &state);
    QWidget *enhanceSelector(const DisplaySettings &state);
    QWidget *saturationSection(const DisplaySettings &state);
    QWidget *experimentalColorSection(const DisplaySettings &state);
    QWidget *unsupportedSaturationSection(bool candidateDisabled = false, bool cloneMode = false);
    QWidget *rgbSection(const DisplaySettings &state);
    QWidget *advancedSection(const DisplaySettings &state);
    QWidget *curveSection(const DisplaySettings &state);
    void addDivider(QVBoxLayout *layout);

    ApplicationController *m_controller;
    DisplayInfo m_info;
    QVBoxLayout *m_layout;
    bool m_rgbExpanded=false;
};

} // namespace eink
