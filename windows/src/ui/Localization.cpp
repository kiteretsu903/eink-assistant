#include "Localization.h"

#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QLocale>
#include <QRegularExpression>
#include <algorithm>

static void initializeEinkResources() { Q_INIT_RESOURCE(resources); }

namespace eink {
namespace {

QString unescape(QString value) {
    value.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    value.replace(QStringLiteral("\\\""), QStringLiteral("\""));
    value.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
    return value;
}

QHash<QString, QString> parseStrings(const QString &path) {
    QFile f(path); QHash<QString,QString> result; if(!f.open(QIODevice::ReadOnly))return result;
    const QString content=QString::fromUtf8(f.readAll());
    const QRegularExpression re(QStringLiteral(R"re("((?:\\.|[^"\\])*)"\s*=\s*"((?:\\.|[^"\\])*)"\s*;)re"));
    auto it=re.globalMatch(content);
    while(it.hasNext()) { const auto m=it.next(); result.insert(unescape(m.captured(1)),unescape(m.captured(2))); }
    return result;
}

QHash<QString,QString> windowsTranslations(const QString &language) {
    if(language==QStringLiteral("zh-Hans"))return {
        {"minimize","最小化"},
        {"system.lightMode","Windows 浅色模式"},{"system.lightMode.note","使用浅色 Windows 界面；推荐用于墨水屏。"},{"system.lightMode.unavailable","需要 Windows 10 2019 年 5 月更新或更高版本。"},
        {"saturation.unavailable","需要 Windows 11 24H2 或更高版本以及支持 ACM 的显示器。"},
        {"saturation.unsupported.upgrade","E-Ink Assistant 暂不支持调整此显示器的饱和度。升级到 Windows 11 24H2 或更高版本可能会增加支持；否则请在 GPU 控制面板中手动调整。"},
        {"saturation.unsupported.driver","当前 GPU/驱动不支持 E-Ink Assistant 调整此显示器的饱和度。请尝试更新显卡驱动，或在 GPU 控制面板中手动调整。"},
        {"saturation.connectedGpu","所连接的 GPU：%1"},{"saturation.open.nvidia","打开 NVIDIA 控制面板"},{"saturation.open.intel","打开 Intel 显卡控制中心"},{"saturation.open.amd","打开 AMD Software"},{"saturation.open.generic","打开 GPU 控制面板"},
        {"saturation.manual","饱和度和 RGB 控制当前未启用；也可在 GPU 控制面板中手动调整。"},{"saturation.unsupported.clone","Windows 无法在复制模式下应用饱和度或 RGB。请切换到扩展模式以使用这些控制。"},{"display.cloneWarning","当前处于复制模式。其他显示调整也会同时影响：%1。"},{"display.internal","内置显示器"},{"display.unknown","显示器"},
        {"saturation.experimental.enable","启用饱和度和 RGB 控制（实验性）"},
        {"saturation.experimental.preparing","将在 %1 秒后启用测试。若屏幕显示异常，稍后会自动回退。"},
        {"saturation.experimental.confirm","请检查屏幕显示。若不确认，将在 %1 秒后自动回退。"},
        {"saturation.experimental.dialog.preparing.title","即将测试显示效果"},{"saturation.experimental.dialog.preparing.body","即将应用轻微饱和度变化，请观察是否出现异常变暗、偏色或内容不可见。"},
        {"saturation.experimental.dialog.confirm.title","显示是否正常？"},{"saturation.experimental.dialog.confirm.body","请确认没有异常变暗、偏色或内容不可见。若有异常请回退；未确认也会自动恢复。"},{"saturation.experimental.seconds","%1 秒"},
        {"saturation.experimental.rollback","回退"},{"saturation.experimental.confirmButton","显示正常，确认启用"},
        {"saturation.acm","使用 Windows 自动色彩管理。退出应用时恢复。"},
        {"shaking.unavailable","此 GPU 在 Windows 上没有安全的公开单显示器抖动控制。"},
        {"night.title","夜间模式"},{"night.note","Windows 的夜间模式是全局设置，不能按显示器控制。"},{"night.open","打开夜间模式设置"},
        {"night.fallback.recommendation","建议在墨水屏上禁用夜间模式。"},{"night.fallback.path","设置 > 系统 > 显示 > 夜间模式"},
        {"night.disable.title","禁用夜间模式"},{"night.disable.note","夜间模式会增加暖色调；墨水屏建议关闭。"},
        {"night.disable.warning","建议在墨水屏上禁用夜间模式。"},
        {"help.windows","饱和度和 RGB 在 Windows 11 24H2 或更高版本上使用自动色彩管理，在 Windows 10 19041–19045 上使用 MHC2 配置文件路径。文本对比度、视频增强和高级模式使用每显示器伽马表。退出应用时恢复所有显示调整，启动时重新应用。"},
        {"welcome.windows.tray.title","固定任务栏图标"},{"welcome.windows.tray","打开任务栏右侧的隐藏图标菜单，找到这个书页图标，再拖到任务栏固定区域。"}
    };
    if(language==QStringLiteral("zh-Hant"))return {
        {"minimize","最小化"},
        {"system.lightMode","Windows 淺色模式"},{"system.lightMode.note","使用淺色 Windows 介面；建議用於電子紙。"},{"system.lightMode.unavailable","需要 Windows 10 2019 年 5 月更新或更新版本。"},
        {"saturation.unavailable","需要 Windows 11 24H2 或更新版本以及支援 ACM 的顯示器。"},
        {"saturation.unsupported.upgrade","E-Ink Assistant 暫不支援調整此顯示器的飽和度。升級至 Windows 11 24H2 或更新版本可能會增加支援；否則請在 GPU 控制台中手動調整。"},
        {"saturation.unsupported.driver","目前的 GPU／驅動程式不支援 E-Ink Assistant 調整此顯示器的飽和度。請嘗試更新顯示卡驅動程式，或在 GPU 控制台中手動調整。"},
        {"saturation.connectedGpu","連接的 GPU：%1"},{"saturation.open.nvidia","開啟 NVIDIA 控制台"},{"saturation.open.intel","開啟 Intel 顯示晶片控制中心"},{"saturation.open.amd","開啟 AMD Software"},{"saturation.open.generic","開啟 GPU 控制台"},
        {"saturation.manual","飽和度和 RGB 控制目前尚未啟用；也可在 GPU 控制台中手動調整。"},{"saturation.unsupported.clone","Windows 無法在同步顯示模式下套用彩度或 RGB。請切換至延伸模式以使用這些控制。"},{"display.cloneWarning","目前處於同步顯示模式。其他顯示調整也會同時影響：%1。"},{"display.internal","內建顯示器"},{"display.unknown","顯示器"},
        {"saturation.experimental.enable","啟用飽和度和 RGB 控制（實驗性）"},
        {"saturation.experimental.preparing","將在 %1 秒後啟用測試。若畫面異常，稍後會自動還原。"},
        {"saturation.experimental.confirm","請檢查畫面。若未確認，將在 %1 秒後自動還原。"},
        {"saturation.experimental.dialog.preparing.title","即將測試顯示效果"},{"saturation.experimental.dialog.preparing.body","即將套用輕微彩度變化，請觀察是否出現異常變暗、偏色或內容無法看見。"},
        {"saturation.experimental.dialog.confirm.title","顯示是否正常？"},{"saturation.experimental.dialog.confirm.body","請確認沒有異常變暗、偏色或內容無法看見。若有異常請還原；未確認也會自動還原。"},{"saturation.experimental.seconds","%1 秒"},
        {"saturation.experimental.rollback","還原"},{"saturation.experimental.confirmButton","顯示正常，確認啟用"},
        {"saturation.acm","使用 Windows 自動色彩管理。結束應用程式時還原。"},
        {"shaking.unavailable","此 GPU 在 Windows 上沒有安全的公開單顯示器抖動控制。"},
        {"night.title","夜間模式"},{"night.note","Windows 的夜間模式是全域設定，無法按顯示器控制。"},{"night.open","開啟夜間模式設定"},
        {"night.fallback.recommendation","建議在電子紙螢幕上停用夜間模式。"},{"night.fallback.path","設定 > 系統 > 顯示器 > 夜間模式"},
        {"night.disable.title","停用夜間模式"},{"night.disable.note","夜間模式會增加暖色調；電子紙建議關閉。"},
        {"night.disable.warning","建議在電子紙螢幕上停用夜間模式。"},
        {"help.windows","飽和度和 RGB 在 Windows 11 24H2 或更新版本上使用自動色彩管理，在 Windows 10 19041–19045 上使用 MHC2 描述檔路徑。文字對比度、影片增強和進階模式使用每顯示器 Gamma 表。結束應用程式時還原所有顯示調整，啟動時重新套用。"},
        {"welcome.windows.tray.title","固定工作列圖示"},{"welcome.windows.tray","開啟工作列右側的隱藏圖示選單，找到這個書頁圖示，再拖到工作列固定區域。"}
    };
    if(language==QStringLiteral("ja"))return {
        {"minimize","最小化"},
        {"system.lightMode","Windows ライト モード"},{"system.lightMode.note","明るい Windows 表示を使用。電子ペーパーにおすすめ。"},{"system.lightMode.unavailable","Windows 10 May 2019 Update 以降が必要です。"},
        {"saturation.unavailable","Windows 11 24H2 以降と ACM 対応ディスプレイが必要です。"},
        {"saturation.unsupported.upgrade","E-Ink Assistant では、このディスプレイの彩度を調整できません。Windows 11 24H2 以降への更新で対応する可能性があります。それ以外の場合は GPU コントロールパネルで手動調整してください。"},
        {"saturation.unsupported.driver","現在の GPU／ドライバーでは、E-Ink Assistant からこのディスプレイの彩度を調整できません。グラフィックスドライバーを更新するか、GPU コントロールパネルで手動調整してください。"},
        {"saturation.connectedGpu","接続先 GPU：%1"},{"saturation.open.nvidia","NVIDIA コントロール パネルを開く"},{"saturation.open.intel","Intel グラフィックスを開く"},{"saturation.open.amd","AMD Software を開く"},{"saturation.open.generic","GPU コントロールパネルを開く"},
        {"saturation.manual","彩度と RGB コントロールは現在無効です。GPU コントロールパネルで手動調整することもできます。"},{"saturation.unsupported.clone","複製表示モードでは Windows が彩度または RGB を適用できません。これらのコントロールを使うには拡張表示に切り替えてください。"},{"display.cloneWarning","現在は複製表示モードです。その他の表示調整は次の画面にも同時に影響します：%1。"},{"display.internal","内蔵ディスプレイ"},{"display.unknown","ディスプレイ"},
        {"saturation.experimental.enable","彩度と RGB コントロールを有効化（試験機能）"},
        {"saturation.experimental.preparing","%1 秒後にテストを開始します。表示に異常があれば自動的に元に戻ります。"},
        {"saturation.experimental.confirm","画面を確認してください。確認しない場合は %1 秒後に自動的に元に戻ります。"},
        {"saturation.experimental.dialog.preparing.title","表示テストを開始します"},{"saturation.experimental.dialog.preparing.body","彩度をわずかに変更します。異常な暗さ、色ずれ、表示の欠落がないか確認してください。"},
        {"saturation.experimental.dialog.confirm.title","表示は正常ですか？"},{"saturation.experimental.dialog.confirm.body","異常な暗さ、色ずれ、表示の欠落がない場合のみ確認してください。異常があれば元に戻してください。未確認の場合も自動的に戻ります。"},{"saturation.experimental.seconds","%1 秒"},
        {"saturation.experimental.rollback","元に戻す"},{"saturation.experimental.confirmButton","正常表示を確認して有効化"},
        {"saturation.acm","Windows の自動カラー管理を使用します。アプリ終了時に元へ戻します。"},
        {"shaking.unavailable","この GPU には、Windows で安全に使える公開のディスプレイ別ディザリング制御がありません。"},
        {"night.title","夜間モード"},{"night.note","Windows の夜間モードはシステム全体の設定で、ディスプレイ別には制御できません。"},{"night.open","夜間モード設定を開く"},
        {"night.fallback.recommendation","電子ペーパーディスプレイでは夜間モードを無効にすることを推奨します。"},{"night.fallback.path","設定 > システム > ディスプレイ > 夜間モード"},
        {"night.disable.title","夜間モードを無効化"},{"night.disable.note","夜間モードは暖色化します。電子ペーパーではオフを推奨。"},
        {"night.disable.warning","電子ペーパーディスプレイでは夜間モードを無効にすることを推奨します。"},
        {"help.windows","彩度と RGB は、Windows 11 24H2 以降では自動カラー管理を、Windows 10 19041～19045 では MHC2 プロファイル経路を使用します。テキストコントラスト、動画補正、詳細設定はディスプレイ別ガンマテーブルを使用します。終了時にすべての表示調整を戻し、起動時に再適用します。"},
        {"welcome.windows.tray.title","トレイアイコンを固定"},{"welcome.windows.tray","タスクバー右側の隠しアイコンメニューを開き、この本のアイコンを表示領域へドラッグします。"}
    };
    return {};
}

void applyWindowsUiFont(const QString &language) {
    if(!qobject_cast<QApplication *>(QCoreApplication::instance()))return;
    QFont font=QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    if(font.family().isEmpty())font=QApplication::font();
    QStringList candidates;
    if(language==QStringLiteral("zh-Hans"))
        candidates=QStringList{QStringLiteral("Microsoft YaHei UI"),QStringLiteral("Microsoft YaHei")};
    else if(language==QStringLiteral("zh-Hant"))
        candidates=QStringList{QStringLiteral("Microsoft JhengHei UI"),QStringLiteral("Microsoft JhengHei")};
    else if(language==QStringLiteral("ja"))
        candidates=QStringList{QStringLiteral("Yu Gothic UI"),QStringLiteral("Meiryo UI"),QStringLiteral("Meiryo")};
    else
        candidates=QStringList{QStringLiteral("Segoe UI"),font.family(),QStringLiteral("Tahoma")};
    const QStringList installed=QFontDatabase().families();
    for(const QString &candidate:candidates) {
        const auto match=std::find_if(installed.cbegin(),installed.cend(),[&](const QString &family){return family.compare(candidate,Qt::CaseInsensitive)==0;});
        if(match!=installed.cend()){font.setFamily(*match);break;}
    }
    // Keep a single script-appropriate UI family for both Latin and CJK text.
    // The transparent composition layer supplies the Chrome-like grayscale edge.
    font.setStyleStrategy(static_cast<QFont::StyleStrategy>(QFont::PreferAntialias|QFont::PreferQuality));
    font.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(font);
}

} // namespace

Localization &Localization::instance() { static Localization value; return value; }

Localization::Localization() {
    initializeEinkResources();
    m_english=parseStrings(QStringLiteral(":/i18n/en.strings"));
    const QHash<QString,QString> windows {
        {"minimize", "Minimize"},
        {"system.lightMode", "Windows Light Mode"},
        {"system.lightMode.note", "Uses a light Windows interface; recommended for e-ink."},
        {"system.lightMode.unavailable", "Requires Windows 10 May 2019 Update or above."},
        {"saturation.unavailable", "Requires Windows 11 24H2 or above and an ACM-capable display."},
        {"saturation.unsupported.upgrade", "Saturation adjustment via E-Ink Assistant is not supported on this display. Windows 11 24H2 or above may add support; otherwise, adjust it in your GPU control panel."},
        {"saturation.unsupported.driver", "Saturation adjustment via E-Ink Assistant is not supported on this display with the current GPU/driver. Try updating the graphics driver or adjust it in the GPU control panel."},
        {"saturation.connectedGpu", "Connected GPU: %1"},
        {"saturation.open.nvidia", "Open NVIDIA Control Panel"},
        {"saturation.open.intel", "Open Intel Graphics"},
        {"saturation.open.amd", "Open AMD Software"},
        {"saturation.open.generic", "Open GPU control panel"},
        {"saturation.manual", "Saturation and RGB controls are off. You can also adjust them manually in the GPU control panel."},{"saturation.unsupported.clone", "Windows cannot apply Saturation or RGB in Duplicate mode. Switch to Extend to use these controls."},{"display.cloneWarning", "Duplicate mode is active. Other display adjustments also affect: %1."},{"display.internal", "Internal Display"},{"display.unknown", "Display"},
        {"saturation.experimental.enable", "Enable Saturation and RGB controls (Experimental)"},
        {"saturation.experimental.preparing", "The test will begin in %1 seconds. If the screen looks wrong, it will automatically roll back."},
        {"saturation.experimental.confirm", "Check the screen. If you do not confirm, it will automatically roll back in %1 seconds."},
        {"saturation.experimental.dialog.preparing.title", "Preparing the display test"},
        {"saturation.experimental.dialog.preparing.body", "A small saturation change will be applied. Watch for abnormal darkening, color shifts, or lost visibility."},
        {"saturation.experimental.dialog.confirm.title", "Does the display look normal?"},
        {"saturation.experimental.dialog.confirm.body", "Confirm only if there is no abnormal darkening, color shift, or loss of visibility. Otherwise roll back; it will also restore automatically."},
        {"saturation.experimental.seconds", "%1 s"},
        {"saturation.experimental.rollback", "Roll back"},
        {"saturation.experimental.confirmButton", "Display looks normal — Enable"},
        {"saturation.acm", "Uses Windows Auto Color Management. Restored when the app quits."},
        {"shaking.unavailable", "No safe public per-display dithering control is available for this GPU on Windows."},
        {"night.title", "Night light"},
        {"night.note", "Windows controls Night light system-wide rather than per display."},
        {"night.open", "Open Night light settings"},
        {"night.fallback.recommendation", "We recommend disabling Night Light for e-ink displays."},
        {"night.fallback.path", "Settings > System > Display > Night light"},
        {"night.disable.title", "Disable Night Light"},
        {"night.disable.note", "Night Light adds a warm tint; turn it off for e-ink."},
        {"night.disable.warning", "For e-ink displays, we recommend disabling Night Light."},
        {"help.windows", "Saturation and RGB use Auto Color Management on Windows 11 24H2 or above, or the MHC2 profile path on Windows 10 builds 19041–19045. Text Contrast, Video Enhance and Advanced use a per-display gamma ramp. All display adjustments are restored when the app quits and re-applied on launch."},
        {"welcome.windows.tray", "Open the hidden-icons menu on the right of the taskbar, find this book-pages icon, then drag it into the visible area."},
        {"welcome.windows.tray.title", "Pin the tray icon"}
    };
    for(auto it=windows.begin();it!=windows.end();++it)m_english.insert(it.key(),it.value());
    setLanguage(QStringLiteral("system"));
}

QString Localization::systemLanguage() {
    const QString name=QLocale::system().name();
    if(name.startsWith(QStringLiteral("zh_TW"))||name.startsWith(QStringLiteral("zh_HK"))||name.startsWith(QStringLiteral("zh_Hant")))return QStringLiteral("zh-Hant");
    if(name.startsWith(QStringLiteral("zh")))return QStringLiteral("zh-Hans");
    if(name.startsWith(QStringLiteral("ja")))return QStringLiteral("ja");
    return QStringLiteral("en");
}

void Localization::setLanguage(const QString &language) {
    m_language=language;const QString resolved=language==QStringLiteral("system")?systemLanguage():language;load(resolved);applyWindowsUiFont(resolved);
}

void Localization::load(const QString &language) {
    m_current=m_english; if(language==QStringLiteral("en"))return;
    const auto translated=parseStrings(QStringLiteral(":/i18n/%1.strings").arg(language));
    for(auto it=translated.begin();it!=translated.end();++it)m_current.insert(it.key(),it.value());
    const auto platform=windowsTranslations(language);
    for(auto it=platform.begin();it!=platform.end();++it)m_current.insert(it.key(),it.value());
}

QString Localization::text(const char *key) const {
    const QString qkey=QString::fromLatin1(key); return m_current.value(qkey,m_english.value(qkey,qkey));
}

} // namespace eink
