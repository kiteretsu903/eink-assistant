#include "DisplayCard.h"
#include "CurvePlot.h"
#include "EinkSwitch.h"
#include "Localization.h"
#include "SmoothLabel.h"
#include "UiStyle.h"

#include <QButtonGroup>
#include <QAbstractScrollArea>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QStyle>
#include <QSlider>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace eink {
namespace {

QString displayLabel(const DisplayInfo &display) {
    QString name=display.friendlyName.trimmed();
    if(display.builtIn&&display.friendlyNameIsFallback)name=L("display.internal");
    else if(name.isEmpty())name=L("display.unknown");
    if(display.builtIn&&!display.friendlyNameIsFallback)name+=QStringLiteral("  ")+L("display.builtin");
    return name;
}

class ScrollFriendlySlider final : public QSlider {
public:
    explicit ScrollFriendlySlider(Qt::Orientation orientation):QSlider(orientation) {}
protected:
    void wheelEvent(QWheelEvent *event) override {
        QWidget *ancestor=parentWidget();
        while(ancestor&&!qobject_cast<QAbstractScrollArea*>(ancestor))ancestor=ancestor->parentWidget();
        auto *area=qobject_cast<QAbstractScrollArea*>(ancestor);
        if(!area){event->ignore();return;}
        QScrollBar *bar=area->verticalScrollBar();
        int movement=-event->pixelDelta().y();
        if(!movement)movement=-(event->angleDelta().y()/120)*bar->singleStep()*3;
        if(movement)bar->setValue(bar->value()+movement);
        event->accept();
    }
};

class DisplaySelector final : public QCheckBox {
public:
    DisplaySelector(const QString &name,bool builtIn,QWidget *parent=nullptr)
        :QCheckBox(name,parent),m_builtIn(builtIn) {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::TabFocus);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        setMinimumHeight(42);
        setAccessibleName(name);
    }
    QSize sizeHint() const override {
        const QFontMetrics metrics(font());
        return {82+metrics.horizontalAdvance(text()),42};
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);
        const QColor ink=isEnabled()?QColor(QStringLiteral("#202020")):QColor(QStringLiteral("#777777"));
        const qreal centerY=height()/2.0;

        const QRectF check(1.5,centerY-13,26,26);
        painter.setPen(QPen(ink,2.2));
        painter.setBrush(isChecked()?QColor(QStringLiteral("#147ee5")):Qt::white);
        painter.drawRoundedRect(check,4,4);
        if(isChecked()) {
            QPainterPath mark;mark.moveTo(7.5,centerY);mark.lineTo(12.0,centerY+5);mark.lineTo(21.5,centerY-6);
            painter.setPen(QPen(Qt::white,2.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));painter.drawPath(mark);
        }

        painter.setPen(QPen(ink,2.0,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));painter.setBrush(Qt::NoBrush);
        if(m_builtIn) {
            const QRectF screen(39.5,centerY-11,24,16);painter.drawRoundedRect(screen,1.8,1.8);
            QPainterPath base;base.moveTo(37.5,centerY+8);base.lineTo(65.5,centerY+8);base.lineTo(62.5,centerY+11);base.lineTo(40.5,centerY+11);base.closeSubpath();painter.drawPath(base);
        } else {
            painter.drawRoundedRect(QRectF(39.5,centerY-11,24,17),1.8,1.8);
            painter.drawLine(QPointF(51.5,centerY+6),QPointF(51.5,centerY+10));
            painter.drawLine(QPointF(45.5,centerY+10),QPointF(57.5,centerY+10));
        }

        painter.setPen(ink);painter.setFont(font());
        const QRect textRect(76,0,qMax(0,width()-76),height());
        const QString elided=fontMetrics().elidedText(text(),Qt::ElideRight,textRect.width());
        style()->drawItemText(&painter,textRect,Qt::AlignLeft|Qt::AlignVCenter,palette(),isEnabled(),elided,QPalette::WindowText);
        if(hasFocus()) {
            QStyleOptionFocusRect focus;focus.initFrom(this);focus.rect=rect().adjusted(0,0,-1,-1);style()->drawPrimitive(QStyle::PE_FrameFocusRect,&focus,&painter,this);
        }
    }
private:
    bool m_builtIn=false;
};

class ChoiceButton final : public QPushButton {
public:
    enum class Position { Standalone, First, Middle, Last };

    ChoiceButton(const QString &text,const QString &name,Position position,QWidget *parent=nullptr)
        :QPushButton(text,parent),m_position(position) {
        setObjectName(name);setCheckable(true);setFocusPolicy(Qt::TabFocus);setCursor(Qt::PointingHandCursor);
        setMinimumHeight(34);setProperty("choice-button",true);setProperty("selected-font-weight",static_cast<int>(QFont::Black));
        const char *positionName=position==Position::Standalone?"standalone":position==Position::First?"first":position==Position::Middle?"middle":"last";
        setProperty("segment-position",positionName);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing,true);
        const QColor ink=isEnabled()?QColor(QStringLiteral("#202020")):QColor(QStringLiteral("#777777"));
        const QColor background=isDown()?QColor(QStringLiteral("#e5f1ff")):(underMouse()?QColor(QStringLiteral("#f0f6ff")):Qt::white);
        const QRectF bounds=QRectF(rect()).adjusted(1.25,1.25,-1.25,-1.25);
        constexpr qreal radius=6.0;

        QPainterPath fill;
        if(m_position==Position::Standalone)fill.addRoundedRect(bounds,radius,radius);
        else if(m_position==Position::First) {
            fill.moveTo(bounds.right(),bounds.top());fill.lineTo(bounds.left()+radius,bounds.top());
            fill.quadTo(bounds.left(),bounds.top(),bounds.left(),bounds.top()+radius);
            fill.lineTo(bounds.left(),bounds.bottom()-radius);fill.quadTo(bounds.left(),bounds.bottom(),bounds.left()+radius,bounds.bottom());
            fill.lineTo(bounds.right(),bounds.bottom());fill.closeSubpath();
        } else if(m_position==Position::Last) {
            fill.moveTo(bounds.left(),bounds.top());fill.lineTo(bounds.right()-radius,bounds.top());
            fill.quadTo(bounds.right(),bounds.top(),bounds.right(),bounds.top()+radius);
            fill.lineTo(bounds.right(),bounds.bottom()-radius);fill.quadTo(bounds.right(),bounds.bottom(),bounds.right()-radius,bounds.bottom());
            fill.lineTo(bounds.left(),bounds.bottom());fill.closeSubpath();
        } else fill.addRect(bounds);
        painter.fillPath(fill,background);

        painter.setPen(QPen(ink,2.0,Qt::SolidLine,Qt::SquareCap,Qt::MiterJoin));painter.setBrush(Qt::NoBrush);
        if(m_position==Position::Standalone)painter.drawRoundedRect(bounds,radius,radius);
        else if(m_position==Position::First) {
            QPainterPath outline;outline.moveTo(bounds.right(),bounds.top());outline.lineTo(bounds.left()+radius,bounds.top());
            outline.quadTo(bounds.left(),bounds.top(),bounds.left(),bounds.top()+radius);outline.lineTo(bounds.left(),bounds.bottom()-radius);
            outline.quadTo(bounds.left(),bounds.bottom(),bounds.left()+radius,bounds.bottom());outline.lineTo(bounds.right(),bounds.bottom());painter.drawPath(outline);
        } else if(m_position==Position::Last) {
            QPainterPath outline;outline.moveTo(bounds.left(),bounds.top());outline.lineTo(bounds.right()-radius,bounds.top());
            outline.quadTo(bounds.right(),bounds.top(),bounds.right(),bounds.top()+radius);outline.lineTo(bounds.right(),bounds.bottom()-radius);
            outline.quadTo(bounds.right(),bounds.bottom(),bounds.right()-radius,bounds.bottom());outline.lineTo(bounds.left(),bounds.bottom());painter.drawPath(outline);
            painter.drawLine(QPointF(bounds.left(),bounds.top()),QPointF(bounds.left(),bounds.bottom()));
        } else {
            painter.drawLine(QPointF(bounds.left(),bounds.top()),QPointF(bounds.right(),bounds.top()));
            painter.drawLine(QPointF(bounds.left(),bounds.bottom()),QPointF(bounds.right(),bounds.bottom()));
            painter.drawLine(QPointF(bounds.left(),bounds.top()),QPointF(bounds.left(),bounds.bottom()));
        }

        if(isChecked()) {
            const qreal inset=m_position==Position::Standalone?6.0:2.0;
            painter.setPen(QPen(ink,4.0,Qt::SolidLine,Qt::FlatCap));
            painter.drawLine(QPointF(bounds.left()+inset,bounds.bottom()-1.0),QPointF(bounds.right()-inset,bounds.bottom()-1.0));
        }

        QFont textFont=font();textFont.setWeight(isChecked()?QFont::Black:QFont::Normal);painter.setFont(textFont);
        style()->drawItemText(&painter,rect().adjusted(6,2,-6,-3),Qt::AlignCenter,palette(),isEnabled(),text(),QPalette::ButtonText);
    }

private:
    Position m_position;
};

class DisclosureButton final : public QPushButton {
public:
    DisclosureButton(const QString &text,bool expanded,QWidget *parent=nullptr)
        :QPushButton(text,parent),m_expanded(expanded) {
        setObjectName(QStringLiteral("rgb-toggle"));setFocusPolicy(Qt::TabFocus);setCursor(Qt::PointingHandCursor);
        setProperty("disclosure-expanded",expanded);setAccessibleName(text);
    }
    QSize sizeHint() const override {QSize result=QPushButton::sizeHint();result.rwidth()+=18;return result;}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);QStyleOptionButton option;initStyleOption(&option);option.text.clear();option.icon=QIcon();
        style()->drawControl(QStyle::CE_PushButton,&option,&painter,this);
        painter.setRenderHint(QPainter::Antialiasing,true);
        const QColor ink=isEnabled()?QColor(QStringLiteral("#202020")):QColor(QStringLiteral("#777777"));
        painter.setPen(QPen(ink,2.4,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        const qreal centerY=height()/2.0;QPainterPath chevron;
        if(m_expanded){chevron.moveTo(11,centerY-3);chevron.lineTo(16,centerY+2);chevron.lineTo(21,centerY-3);}
        else {chevron.moveTo(13,centerY-5);chevron.lineTo(18,centerY);chevron.lineTo(13,centerY+5);}
        painter.drawPath(chevron);
        style()->drawItemText(&painter,rect().adjusted(28,0,-8,0),Qt::AlignLeft|Qt::AlignVCenter,palette(),isEnabled(),text(),QPalette::ButtonText);
    }
private:
    bool m_expanded=false;
};

QLabel *valueLabel(const QString &text) {
    auto *v=new SmoothLabel(text); v->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    QFont f=v->font(); f.setWeight(QFont::DemiBold); f.setPointSize(13); v->setFont(f); return v;
}

QSlider *slider(int minimum,int maximum,int value,const QString &name) {
    auto *s=new ScrollFriendlySlider(Qt::Horizontal); s->setRange(minimum,maximum); s->setValue(value); s->setObjectName(name); s->setMinimumHeight(34); return s;
}

} // namespace

DisplayCard::DisplayCard(ApplicationController *controller, DisplayInfo info, bool rgbExpanded, QWidget *parent)
    :QFrame(parent),m_controller(controller),m_info(std::move(info)),m_rgbExpanded(rgbExpanded) {
    setObjectName(QStringLiteral("displayCard")); setFrameShape(QFrame::NoFrame);
    setStyleSheet(QStringLiteral("QFrame#displayCard { border:2px solid #202020; border-radius:7px; background:white; } QFrame#displayCard QLabel { border:0; }"));
    m_layout=new QVBoxLayout(this);m_layout->setSizeConstraint(QLayout::SetMinimumSize);m_layout->setContentsMargins(14,14,14,14);m_layout->setSpacing(12);
    connect(m_controller,&ApplicationController::colorSafetyStateChanged,this,[this](const QString &id,ColorSafetyPhase phase,int){
        if(id!=m_info.stableId)return;
        for(auto *toggle:findChildren<QAbstractButton*>(QStringLiteral("experimental-color-switch"))) {
            if(toggle->property("_einkPendingDelete").toBool())continue;
            const QSignalBlocker blocker(toggle);
            toggle->setChecked(m_controller->colorControlsEnabled(id)||phase!=ColorSafetyPhase::Idle);
            toggle->setEnabled(phase==ColorSafetyPhase::Idle);
        }
        QTimer::singleShot(0,this,&DisplayCard::rebuild);
    });
    rebuild();
}

QLabel *DisplayCard::label(const QString &text,bool heading,bool secondary) {
    auto *l=new SmoothLabel(text); l->setWordWrap(true); if(heading)l->setProperty("heading",true); if(secondary)l->setProperty("secondary",true); return l;
}

void DisplayCard::addDivider(QVBoxLayout *layout) { layout->addWidget(ui::divider()); }

void DisplayCard::rebuild() {
    ui::clearLayout(m_layout); DisplaySettings &state=m_controller->settingsFor(m_info.stableId);
    auto *check=new DisplaySelector(displayLabel(m_info),m_info.builtIn);
    check->setObjectName(QStringLiteral("eink-checkbox")); check->setProperty("displayId",m_info.stableId); check->setChecked(state.isEink);
    connect(check,&QCheckBox::toggled,this,[this](bool on){m_controller->setEink(m_info.stableId,on);QTimer::singleShot(0,this,&DisplayCard::rebuild);}); m_layout->addWidget(check);
    if(!state.isEink){m_layout->invalidate();updateGeometry();emit contentSizeChanged();return;}
    if(m_info.cloneMode) {
        QStringList peerNames;
        for(const DisplayInfo &peer:m_controller->displays())
            if(peer.stableId!=m_info.stableId&&peer.cloneGroupKey==m_info.cloneGroupKey)peerNames.push_back(displayLabel(peer));
        if(peerNames.isEmpty())peerNames=m_info.clonePeerNames;
        QString warning=L("display.cloneWarning");warning.replace(QStringLiteral("%1"),peerNames.join(QStringLiteral(", ")));
        auto *cloneWarning=label(QStringLiteral("⚠ ")+warning);cloneWarning->setObjectName(QStringLiteral("clone-mode-warning"));
        cloneWarning->setStyleSheet(QStringLiteral("color:#a64b00;font-weight:400;"));m_layout->addWidget(cloneWarning);addDivider(m_layout);
    }

    const bool colorEnabled=m_controller->colorControlsEnabled(m_info.stableId);
    const bool experimentAvailable=m_controller->colorExperimentAvailable(m_info.stableId);
    if(m_info.cloneMode) {
        m_layout->addWidget(unsupportedSaturationSection(false,true));addDivider(m_layout);
    } else if(m_info.usesWindows10Mhc2&&(experimentAvailable||colorEnabled)) {
        m_layout->addWidget(experimentalColorSection(state));addDivider(m_layout);
        if(colorEnabled){m_layout->addWidget(saturationSection(state));addDivider(m_layout);m_layout->addWidget(rgbSection(state));addDivider(m_layout);}
        else {m_layout->addWidget(unsupportedSaturationSection(true));addDivider(m_layout);}
    } else if(colorEnabled){m_layout->addWidget(saturationSection(state));addDivider(m_layout);m_layout->addWidget(rgbSection(state));addDivider(m_layout);}
    else {m_layout->addWidget(unsupportedSaturationSection());addDivider(m_layout);}
    if(!state.advanced){m_layout->addWidget(textSelector(state));addDivider(m_layout);m_layout->addWidget(enhanceSelector(state));addDivider(m_layout);}
    m_layout->addWidget(advancedSection(state)); addDivider(m_layout); m_layout->addWidget(curveSection(state));
    if(isVisible())for(QWidget *child:findChildren<QWidget*>())if(!child->property("_einkPendingDelete").toBool())child->show();
    m_layout->invalidate();m_layout->activate();updateGeometry();emit contentSizeChanged();
}

QWidget *DisplayCard::saturationSection(const DisplaySettings &state) {
    auto *w=new QWidget; auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(6);
    auto *head=new QHBoxLayout;head->addWidget(label(L("saturation.title"),true));head->addStretch();auto *percent=valueLabel(QStringLiteral("%1%").arg(qRound(state.saturation*100)));percent->setObjectName(QStringLiteral("saturation-value"));head->addWidget(percent);v->addLayout(head);
    const bool available=m_controller->platform().saturationPlatformAvailable()&&m_info.colorAdjustmentSupported;
    auto *s=slider(0,300,qRound(state.saturation*100),QStringLiteral("saturation-slider"));s->setEnabled(available);connect(s,&QSlider::valueChanged,percent,[percent](int value){percent->setText(QStringLiteral("%1%").arg(value));});connect(s,&QSlider::sliderReleased,this,[this,s]{m_controller->setSaturation(m_info.stableId,s->value()/100.0,-1);});v->addWidget(s);
    auto *presets=new QHBoxLayout;auto *presetGroup=new QButtonGroup(w);presetGroup->setExclusive(true); const int values[]={0,50,100,130,150,200}; const char *keys[]={"preset.bw","preset.faded","preset.factory","preset.enhanced","preset.vivid","preset.anime"}; const char *names[]={"preset-bw","preset-faded","preset-factory","preset-enhanced","preset-vivid","preset-anime"};
    for(int i=0;i<6;++i){auto *b=new ChoiceButton(L(keys[i]),QString::fromLatin1(names[i]),ChoiceButton::Position::Standalone);b->setEnabled(available);b->setChecked(state.saturationPreset==i);QFont checkedFont=b->font();checkedFont.setWeight(QFont::Black);const int checkedWidth=QFontMetrics(checkedFont).horizontalAdvance(b->text())+30;b->setMinimumWidth(checkedWidth);presetGroup->addButton(b,i);connect(b,&QPushButton::clicked,this,[this,i,values,s]{s->setValue(values[i]);m_controller->setSaturation(m_info.stableId,values[i]/100.0,i);});presets->addWidget(b,1);}v->addLayout(presets);return w;
}

QWidget *DisplayCard::experimentalColorSection(const DisplaySettings &) {
    auto *w=new QWidget;w->setObjectName(QStringLiteral("experimental-color-section"));auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(7);
    const ColorSafetyPhase phase=m_controller->colorSafetyPhase(m_info.stableId);
    const bool enabled=m_controller->colorControlsEnabled(m_info.stableId);
    auto *row=new QHBoxLayout;auto *title=label(L("saturation.experimental.enable"),true);title->setWordWrap(false);row->addWidget(title);row->addStretch();
    auto *toggle=new EinkSwitch;toggle->setObjectName(QStringLiteral("experimental-color-switch"));toggle->setProperty("displayId",m_info.stableId);toggle->setChecked(enabled||phase!=ColorSafetyPhase::Idle);toggle->setEnabled(phase==ColorSafetyPhase::Idle);
    connect(toggle,&QAbstractButton::toggled,this,[this](bool on){m_controller->setExperimentalColorEnabled(m_info.stableId,on);});row->addWidget(toggle);v->addLayout(row);
    return w;
}

QWidget *DisplayCard::unsupportedSaturationSection(bool candidateDisabled,bool cloneMode) {
    auto *w=new QWidget;w->setObjectName(QStringLiteral("saturation-unsupported"));auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(7);
    v->addWidget(label(L("saturation.title"),true));
    const bool upgradeMayHelp=m_info.colorAdjustmentUpgradeMayHelp&&!m_controller->colorExperimentDenied(m_info.stableId);
    auto *note=label(cloneMode?L("saturation.unsupported.clone"):(candidateDisabled?L("saturation.manual"):(upgradeMayHelp?L("saturation.unsupported.upgrade"):L("saturation.unsupported.driver"))),false,true);
    note->setObjectName(QStringLiteral("saturation-unsupported-note"));v->addWidget(note);
    if(!cloneMode&&!m_info.graphicsAdapterName.isEmpty()) {
        QString adapterText=L("saturation.connectedGpu");adapterText.replace(QStringLiteral("%1"),m_info.graphicsAdapterName);
        auto *adapter=label(adapterText,false,true);adapter->setObjectName(QStringLiteral("saturation-gpu-name"));v->addWidget(adapter);
    }
    if(!cloneMode&&m_info.gpuControlPanelAvailable) {
        const char *key=m_info.graphicsVendor==GraphicsVendor::Nvidia?"saturation.open.nvidia"
            :m_info.graphicsVendor==GraphicsVendor::Intel?"saturation.open.intel"
            :m_info.graphicsVendor==GraphicsVendor::Amd?"saturation.open.amd":"saturation.open.generic";
        auto *row=new QHBoxLayout;auto *open=ui::outlinedButton(L(key),QStringLiteral("gpu-control-panel-button"));
        open->setProperty("displayId",m_info.stableId);open->setProperty("graphicsVendor",static_cast<int>(m_info.graphicsVendor));
        connect(open,&QPushButton::clicked,this,[this]{m_controller->openGpuControlPanel(m_info.stableId);});row->addWidget(open);row->addStretch();v->addLayout(row);
    }
    return w;
}

QWidget *DisplayCard::rgbSection(const DisplaySettings &state) {
    auto *w=new QWidget;auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(6);auto *head=new QHBoxLayout;
    auto *toggle=new DisclosureButton(L("rgb.title"),m_rgbExpanded);connect(toggle,&QPushButton::clicked,this,[this]{m_rgbExpanded=!m_rgbExpanded;emit rgbExpansionChanged(m_info.stableId,m_rgbExpanded);rebuild();});head->addWidget(toggle);head->addStretch();
    QPushButton *reset=nullptr;
    if(!m_rgbExpanded)head->addWidget(valueLabel(QStringLiteral("R %1%  G %2%  B %3%").arg(qRound(state.rgb.red*100)).arg(qRound(state.rgb.green*100)).arg(qRound(state.rgb.blue*100))));
    else {reset=ui::outlinedButton(L("rgb.reset"),QStringLiteral("rgb-reset"));reset->setEnabled(!state.rgb.isIdentity());connect(reset,&QPushButton::clicked,this,[this,w,reset]{for(const char *name:{"rgb-red","rgb-green","rgb-blue"})if(auto *s=w->findChild<QSlider*>(QString::fromLatin1(name)))s->setValue(100);reset->setEnabled(false);m_controller->setRgb(m_info.stableId,RgbBalance{});});head->addWidget(reset);}v->addLayout(head);
    if(m_rgbExpanded){
        const auto updateReset=[w,reset]{const auto *r=w->findChild<QSlider*>(QStringLiteral("rgb-red"));const auto *g=w->findChild<QSlider*>(QStringLiteral("rgb-green"));const auto *b=w->findChild<QSlider*>(QStringLiteral("rgb-blue"));reset->setEnabled((r&&r->value()!=100)||(g&&g->value()!=100)||(b&&b->value()!=100));};
        auto add=[&](const char *key,const char *name,double value,int channel){auto *row=new QHBoxLayout;auto *nameLabel=label(L(key),true);nameLabel->setFixedWidth(55);row->addWidget(nameLabel);auto *s=slider(0,200,qRound(value*100),QString::fromLatin1(name));row->addWidget(s,1);auto *val=valueLabel(QStringLiteral("%1%").arg(qRound(value*100)));val->setFixedWidth(48);row->addWidget(val);connect(s,&QSlider::valueChanged,val,[val,updateReset](int x){val->setText(QStringLiteral("%1%").arg(x));updateReset();});connect(s,&QSlider::sliderReleased,this,[this,s,channel]{RgbBalance rgb=m_controller->settingsFor(m_info.stableId).rgb;double x=s->value()/100.0;if(channel==0)rgb.red=x;else if(channel==1)rgb.green=x;else rgb.blue=x;m_controller->setRgb(m_info.stableId,rgb);});v->addLayout(row);};
        add("rgb.red","rgb-red",state.rgb.red,0);add("rgb.green","rgb-green",state.rgb.green,1);add("rgb.blue","rgb-blue",state.rgb.blue,2);
    }
    return w;
}

QWidget *DisplayCard::textSelector(const DisplaySettings &state) {
    auto *w=new QWidget;auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(6);v->addWidget(label(L("text.title"),true));auto *row=new QHBoxLayout;row->setSpacing(0);auto *group=new QButtonGroup(w);group->setExclusive(true);const char *keys[]={"level.off","level.medium","level.strong","level.sharp","level.solid"};
    for(int i=0;i<5;++i){const auto position=i==0?ChoiceButton::Position::First:(i==4?ChoiceButton::Position::Last:ChoiceButton::Position::Middle);auto *b=new ChoiceButton(L(keys[i]),QStringLiteral("text-")+QString::fromLatin1(keys[i]+6),position);b->setChecked(static_cast<int>(state.textLevel)==i);group->addButton(b,i);connect(b,&QPushButton::clicked,this,[this,i]{m_controller->setTextLevel(m_info.stableId,static_cast<TextLevel>(i));QTimer::singleShot(0,this,&DisplayCard::rebuild);});row->addWidget(b,1);}v->addLayout(row);
    auto *caption=label(L("text.caption"),false,true);caption->setObjectName(QStringLiteral("text-caption"));v->addWidget(caption);
    const char *details[]={nullptr,"text.detail.medium","text.detail.strong","text.detail.sharp","text.detail.solid"};const int selected=static_cast<int>(state.textLevel);
    if(selected>0&&selected<5){auto *detail=label(L(details[selected]),false,true);detail->setObjectName(QStringLiteral("text-detail"));v->addWidget(detail);}
    return w;
}

QWidget *DisplayCard::enhanceSelector(const DisplaySettings &state) {
    auto *w=new QWidget;auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(6);v->addWidget(label(L("video.title"),true));auto *row=new QHBoxLayout;row->setSpacing(0);auto *group=new QButtonGroup(w);group->setExclusive(true);const char *keys[]={"level.off","level.subtle","level.medium","level.strong"};
    for(int i=0;i<4;++i){const auto position=i==0?ChoiceButton::Position::First:(i==3?ChoiceButton::Position::Last:ChoiceButton::Position::Middle);auto *b=new ChoiceButton(L(keys[i]),QStringLiteral("video-")+QString::fromLatin1(keys[i]+6),position);b->setChecked(static_cast<int>(state.enhanceLevel)==i);group->addButton(b,i);connect(b,&QPushButton::clicked,this,[this,i]{m_controller->setEnhanceLevel(m_info.stableId,static_cast<EnhanceLevel>(i));QTimer::singleShot(0,this,&DisplayCard::rebuild);});row->addWidget(b,1);}v->addLayout(row);
    auto *caption=label(L("video.caption"),false,true);caption->setObjectName(QStringLiteral("video-caption"));v->addWidget(caption);
    const int selected=static_cast<int>(state.enhanceLevel);
    if(selected>0&&selected<4){const char *costKeys[]={nullptr,"cost.13","cost.34","cost.56"};QString warning=L("video.warning");warning.replace(QStringLiteral("%@"),L(costKeys[selected]));auto *message=label(QStringLiteral("⚠ ")+warning);message->setObjectName(QStringLiteral("video-warning"));message->setStyleSheet(QStringLiteral("color:#a64b00;"));v->addWidget(message);}
    else if(state.textLevel!=TextLevel::Off){auto *blocked=label(L("video.blocked"),false,true);blocked->setObjectName(QStringLiteral("video-blocked"));v->addWidget(blocked);}
    return w;
}

QWidget *DisplayCard::advancedSection(const DisplaySettings &state) {
    auto *w=new QWidget;auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(7);auto *head=new QHBoxLayout;head->addWidget(label(L("advanced.title"),true));head->addStretch();auto *toggle=new EinkSwitch;toggle->setObjectName(QStringLiteral("advanced-switch"));toggle->setChecked(state.advanced);connect(toggle,&QAbstractButton::toggled,this,[this](bool on){m_controller->setAdvanced(m_info.stableId,on);QTimer::singleShot(0,this,&DisplayCard::rebuild);});head->addWidget(toggle);v->addLayout(head);if(!state.advanced)return w;v->addWidget(label(L("advanced.note"),false,true));
    auto add=[&](const char *key,const char *name,double value,double lo,double hi,int field){auto *title=new QHBoxLayout;title->addWidget(label(L(key)));title->addStretch();auto *val=valueLabel(QString::number(value,'f',2));title->addWidget(val);v->addLayout(title);auto *s=slider(qRound(lo*100),qRound(hi*100),qRound(value*100),QString::fromLatin1(name));connect(s,&QSlider::valueChanged,val,[val](int x){val->setText(QString::number(x/100.0,'f',2));});connect(s,&QSlider::sliderReleased,this,[this,s,field]{ToneCurve c=m_controller->settingsFor(m_info.stableId).customCurve;double x=s->value()/100.0;if(field==0)c.knee=x;else if(field==1)c.gamma=x;else if(field==2)c.blackPoint=x;else c.whitePoint=x;m_controller->setCustomCurve(m_info.stableId,c);});v->addWidget(s);};
    add("curve.knee","curve-knee",state.customCurve.knee,.05,1,0);add("curve.gamma","curve-gamma",state.customCurve.gamma,.30,6,1);add("curve.black","curve-black",state.customCurve.blackPoint,0,.40,2);add("curve.white","curve-white",state.customCurve.whitePoint,.60,1,3);
    auto *resetRow=new QHBoxLayout;resetRow->addStretch();auto *reset=ui::outlinedButton(L("advanced.reset"),QStringLiteral("curve-reset"));connect(reset,&QPushButton::clicked,this,[this]{m_controller->setCustomCurve(m_info.stableId,ToneCurve::identity());QTimer::singleShot(0,this,&DisplayCard::rebuild);});resetRow->addWidget(reset);v->addLayout(resetRow);
    v->addWidget(label(L("presets.title"),true));auto *slotLayout=new QHBoxLayout;
    for(int i=0;i<5;++i){const SavedCurve saved=m_controller->settings().savedCurves.value(i);auto *b=ui::outlinedButton(saved.occupied?(saved.name.isEmpty()?QString::number(i+1):saved.name):QStringLiteral("+"),QStringLiteral("curve-slot-%1").arg(i+1));connect(b,&QPushButton::clicked,this,[this,i,saved]{if(saved.occupied)m_controller->applySavedCurve(i,m_info.stableId);else m_controller->saveCurve(i,m_controller->settingsFor(m_info.stableId).customCurve);});b->setContextMenuPolicy(Qt::CustomContextMenu);connect(b,&QWidget::customContextMenuRequested,this,[this,b,i,saved](const QPoint &p){if(!saved.occupied)return;QMenu menu;auto *rename=menu.addAction(L("presets.rename"));auto *overwrite=menu.addAction(L("presets.overwrite"));auto *clear=menu.addAction(L("presets.clear"));QAction *chosen=menu.exec(b->mapToGlobal(p));if(chosen==rename){bool ok=false;QString name=QInputDialog::getText(this,L("presets.rename"),L("presets.rename"),QLineEdit::Normal,saved.name,&ok);if(ok)m_controller->renameCurve(i,name);}else if(chosen==overwrite)m_controller->saveCurve(i,m_controller->settingsFor(m_info.stableId).customCurve);else if(chosen==clear)m_controller->clearCurve(i);});slotLayout->addWidget(b,1);}v->addLayout(slotLayout);v->addWidget(label(L("presets.hint"),false,true));return w;
}

QWidget *DisplayCard::curveSection(const DisplaySettings &state) {
    auto *w=new QWidget;auto *v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,0);v->setSpacing(6);auto *head=new QHBoxLayout;head->addWidget(label(L("curve.title"),true));head->addStretch();QString mode=L("level.off");if(state.advanced)mode=L("advanced.title");else if(state.textLevel!=TextLevel::Off)mode=L("text.title");else if(state.enhanceLevel!=EnhanceLevel::Off)mode=L("video.short");head->addWidget(label(mode,false,true));v->addLayout(head);auto *plot=new CurvePlot;plot->setCurve(state.effectiveCurve());v->addWidget(plot);return w;
}

} // namespace eink
