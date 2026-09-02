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
        {"system.lightMode","Windows 浅色模式"},{"system.lightMode.note","仅更改 Windows 模式，不更改应用模式。不会跟随墨水屏，也不会在应用退出时恢复。"},{"system.lightMode.unavailable","需要 Windows 10 2019 年 5 月更新或更高版本。"},
        {"saturation.unavailable","需要 Windows 11 24H2 或更高版本以及支持 ACM 的显示器。"},
        {"saturation.acm","使用 Windows 自动色彩管理。退出应用时恢复。"},
        {"shaking.unavailable","此 GPU 在 Windows 上没有安全的公开单显示器抖动控制。"},
        {"night.title","夜间模式"},{"night.note","Windows 的夜间模式是全局设置，不能按显示器控制。"},{"night.open","打开夜间模式设置"},
        {"night.fallback.recommendation","建议在墨水屏上禁用夜间模式。"},{"night.fallback.path","设置 > 系统 > 显示 > 夜间模式"},
        {"night.disable.title","禁用夜间模式"},{"night.disable.note","与 Windows 当前状态同步：此处关闭代表夜间模式已开启。本次运行中作出的更改会在应用退出时恢复。"},
        {"night.disable.warning","建议在墨水屏上禁用夜间模式。"},
        {"help.windows","饱和度和 RGB 在 Windows 11 24H2 或更高版本上使用自动色彩管理，在 Windows 10 19041–19045 上使用 MHC2 配置文件路径。文本对比度、视频增强和高级模式使用每显示器伽马表。退出应用时恢复所有显示调整，启动时重新应用。"},
        {"welcome.windows.tray.title","系统托盘"},{"welcome.windows.tray","书本图标位于通知区域。若 Windows 10 将它放在 ^ 菜单中，请将它拖到任务栏一次；之后点击它即可重新打开控制面板。"},
        {"welcome.windows.other","黑白和彩色电子墨水显示器都可使用文本对比度、视频增强、高级曲线和视觉效果控制。Windows 10 19041–19045 或 Windows 11 24H2 及更高版本的彩色面板还可使用饱和度和 RGB。"}
    };
    if(language==QStringLiteral("zh-Hant"))return {
        {"minimize","最小化"},
        {"system.lightMode","Windows 淺色模式"},{"system.lightMode.note","只變更 Windows 模式，不變更應用程式模式。不會跟隨電子紙，也不會在應用程式結束時還原。"},{"system.lightMode.unavailable","需要 Windows 10 2019 年 5 月更新或更新版本。"},
        {"saturation.unavailable","需要 Windows 11 24H2 或更新版本以及支援 ACM 的顯示器。"},
        {"saturation.acm","使用 Windows 自動色彩管理。結束應用程式時還原。"},
        {"shaking.unavailable","此 GPU 在 Windows 上沒有安全的公開單顯示器抖動控制。"},
        {"night.title","夜間模式"},{"night.note","Windows 的夜間模式是全域設定，無法按顯示器控制。"},{"night.open","開啟夜間模式設定"},
        {"night.fallback.recommendation","建議在電子紙螢幕上停用夜間模式。"},{"night.fallback.path","設定 > 系統 > 顯示器 > 夜間模式"},
        {"night.disable.title","停用夜間模式"},{"night.disable.note","與 Windows 目前狀態同步：此處關閉代表夜間模式已開啟。本次執行中所做的變更會在應用程式結束時還原。"},
        {"night.disable.warning","建議在電子紙螢幕上停用夜間模式。"},
        {"help.windows","飽和度和 RGB 在 Windows 11 24H2 或更新版本上使用自動色彩管理，在 Windows 10 19041–19045 上使用 MHC2 描述檔路徑。文字對比度、影片增強和進階模式使用每顯示器 Gamma 表。結束應用程式時還原所有顯示調整，啟動時重新套用。"},
        {"welcome.windows.tray.title","系統匣"},{"welcome.windows.tray","書本圖示位於通知區域。若 Windows 10 將它放在 ^ 選單中，請將它拖到工作列一次；之後按一下即可重新開啟控制面板。"},
        {"welcome.windows.other","黑白和彩色電子紙顯示器都可使用文字對比度、影片增強、進階曲線和視覺效果控制。Windows 10 19041–19045 或 Windows 11 24H2 及更新版本的彩色面板還可使用飽和度和 RGB。"}
    };
    if(language==QStringLiteral("ja"))return {
        {"minimize","最小化"},
        {"system.lightMode","Windows ライト モード"},{"system.lightMode.note","Windows モードだけを変更し、アプリ モードは変更しません。電子ペーパーには追従せず、アプリ終了時にも元に戻しません。"},{"system.lightMode.unavailable","Windows 10 May 2019 Update 以降が必要です。"},
        {"saturation.unavailable","Windows 11 24H2 以降と ACM 対応ディスプレイが必要です。"},
        {"saturation.acm","Windows の自動カラー管理を使用します。アプリ終了時に元へ戻します。"},
        {"shaking.unavailable","この GPU には、Windows で安全に使える公開のディスプレイ別ディザリング制御がありません。"},
        {"night.title","夜間モード"},{"night.note","Windows の夜間モードはシステム全体の設定で、ディスプレイ別には制御できません。"},{"night.open","夜間モード設定を開く"},
        {"night.fallback.recommendation","電子ペーパーディスプレイでは夜間モードを無効にすることを推奨します。"},{"night.fallback.path","設定 > システム > ディスプレイ > 夜間モード"},
        {"night.disable.title","夜間モードを無効化"},{"night.disable.note","Windows の現在の状態と同期します。ここがオフなら夜間モードはオンです。この実行中の変更はアプリ終了時に元へ戻します。"},
        {"night.disable.warning","電子ペーパーディスプレイでは夜間モードを無効にすることを推奨します。"},
        {"help.windows","彩度と RGB は、Windows 11 24H2 以降では自動カラー管理を、Windows 10 19041～19045 では MHC2 プロファイル経路を使用します。テキストコントラスト、動画補正、詳細設定はディスプレイ別ガンマテーブルを使用します。終了時にすべての表示調整を戻し、起動時に再適用します。"},
        {"welcome.windows.tray.title","システムトレイ"},{"welcome.windows.tray","本のアイコンは通知領域にあります。Windows 10 で ^ メニュー内にある場合は、一度タスクバーへドラッグしてください。以後はクリックしてコントロールパネルを開けます。"},
        {"welcome.windows.other","白黒とカラーの電子ペーパーディスプレイで、テキストコントラスト、動画補正、詳細カーブ、視覚効果制御を利用できます。Windows 10 19041～19045 または Windows 11 24H2 以降のカラーパネルでは彩度と RGB も利用できます。"}
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
        {"system.lightMode.note", "Changes only Windows mode, not app mode. It does not follow e-ink displays and is not restored when the app quits."},
        {"system.lightMode.unavailable", "Requires Windows 10 May 2019 Update or above."},
        {"saturation.unavailable", "Requires Windows 11 24H2 or above and an ACM-capable display."},
        {"saturation.acm", "Uses Windows Auto Color Management. Restored when the app quits."},
        {"shaking.unavailable", "No safe public per-display dithering control is available for this GPU on Windows."},
        {"night.title", "Night light"},
        {"night.note", "Windows controls Night light system-wide rather than per display."},
        {"night.open", "Open Night light settings"},
        {"night.fallback.recommendation", "We recommend disabling Night Light for e-ink displays."},
        {"night.fallback.path", "Settings > System > Display > Night light"},
        {"night.disable.title", "Disable Night Light"},
        {"night.disable.note", "Matches the current Windows state: Off here means Night Light is on. Changes made during this session are restored when the app quits."},
        {"night.disable.warning", "For e-ink displays, we recommend disabling Night Light."},
        {"help.windows", "Saturation and RGB use Auto Color Management on Windows 11 24H2 or above, or the MHC2 profile path on Windows 10 builds 19041–19045. Text Contrast, Video Enhance and Advanced use a per-display gamma ramp. All display adjustments are restored when the app quits and re-applied on launch."},
        {"welcome.windows.tray", "The book icon is in the notification area. If Windows 10 keeps it under the ^ menu, drag it onto the taskbar once. Click it later to reopen the control panel."},
        {"welcome.windows.tray.title", "System tray"},
        {"welcome.windows.other", "B&W and color e-ink displays can use Text Contrast, Video Enhance, Advanced curves and visual-effects controls. Color panels can also use Saturation and RGB on Windows 10 builds 19041–19045 or Windows 11 24H2 or above."}
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
