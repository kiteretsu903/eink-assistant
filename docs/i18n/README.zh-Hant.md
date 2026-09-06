# E-Ink Assistant

<!-- BEGIN README LANGUAGES -->
<p align="center">
  <a href="../../README.md" lang="en" dir="ltr">English</a> &nbsp;·&nbsp;
  <a href="README.zh-Hans.md" lang="zh-Hans" dir="ltr">简体中文</a> &nbsp;·&nbsp;
  <b lang="zh-Hant" dir="ltr">繁體中文</b> &nbsp;·&nbsp;
  <a href="README.ja.md" lang="ja" dir="ltr">日本語</a> &nbsp;·&nbsp;
  <a href="README.ko.md" lang="ko" dir="ltr">한국어</a> &nbsp;·&nbsp;
  <a href="README.es.md" lang="es" dir="ltr">Español</a> &nbsp;·&nbsp;
  <a href="README.fr.md" lang="fr" dir="ltr">Français</a> &nbsp;·&nbsp;
  <a href="README.de.md" lang="de" dir="ltr">Deutsch</a> &nbsp;·&nbsp;
  <a href="README.pt-BR.md" lang="pt-BR" dir="ltr">Português (Brasil)</a> &nbsp;·&nbsp;
  <a href="README.ru.md" lang="ru" dir="ltr">Русский</a> &nbsp;·&nbsp;
  <a href="README.ar.md" lang="ar" dir="rtl">العربية</a> &nbsp;·&nbsp;
  <a href="README.hi.md" lang="hi" dir="ltr">हिन्दी</a>
</p>
<!-- END README LANGUAGES -->

<p align="center">
  <img src="../../Resources/AppIcon.png" alt="E-Ink Assistant App 圖示" width="128">
</p>

**在 macOS 與 Windows 上調校黑白及彩色電子紙顯示器。**

[瀏覽產品網站](https://kiteretsu903.github.io/eink-assistant/zh-Hant/)

E-Ink Assistant 為你選取的電子紙顯示器調整文字對比度、暗部細節與色彩。
其他顯示器維持原狀。macOS 版在選單列執行；Windows 版在系統匣執行。

[下載 macOS 2.6](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-v2.6.dmg) ·
[下載 Windows 1.2](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-Windows-1.2-Setup.exe) ·
[查看所有版本](https://github.com/kiteretsu903/eink-assistant/releases)

免費、開放原始碼，採用 MIT 授權。

## 功能與系統需求

| 功能 | macOS | Windows |
|---|---|---|
| 支援的系統 | **macOS 14 或更新版本**<br>僅限 Apple silicon | **Windows 7 SP1 至 Windows 11**<br>x64 電腦 |
| App 執行位置 | 選單列 | 系統匣 |
| 選取特定電子紙顯示器 | 支援。其他顯示器維持原狀。 | 與 macOS 相同 |
| 文字對比度 | 四種等級：中等、強、銳利、實邊 | 與 macOS 相同 |
| 影片暗部增強 | 三種等級：輕微、中等、強 | 與 macOS 相同 |
| 進階曲線與預設 | 即時曲線編輯器，以及五組具名稱的預設 | 與 macOS 相同 |
| 彩度與 RGB | 各顯示器獨立的色彩描述檔；彩度 0%–300%，RGB 0%–200% | 適用於符合條件的 Windows 10 2004 與 Windows 11 21H2+ 系統；可用方式取決於系統與硬體 |
| 減少閃爍 | 適用於支援的外接顯示器；將顯示器標記為電子紙時自動開啟 | 不提供。Windows 沒有統一的公開介面可控制單一顯示器的抖動，而且大多數 Windows 系統可能不需要這項功能。 |
| 降低透明度與動態效果 | 透過使用者確認安裝一次的輔助捷徑提供 | Windows 7 SP1 起可透過相容的系統 API 使用 |
| 系統淺色模式 | 不變更 | Windows 10 1903+ 提供僅在本次執行期間生效的 Windows 淺色模式 |
| Night Shift／夜間模式 | 可將個別顯示器排除於 Night Shift 與原彩之外；需要管理員核准並重新連接 | Windows 10 1703+ 可開啟夜間模式設定；Windows 11 24H2+ 可直接使用「停用夜間模式」控制項 |
| 鏡像／同步顯示器 | 鏡像模式下仍可個別選取實體顯示器 | 色調曲線會影響共用的訊號來源；彩度與 RGB 需要延伸模式 |
| 還原變更 | 結束時還原暫時套用的曲線、色彩描述檔與抖動設定；Night Shift／原彩排除設定會持續保留 | 結束時還原暫時套用的 Gamma、色彩、視覺效果與夜間模式變更；異常結束後也能復原色彩與夜間模式 |
| 登入時啟動 | 支援 | 支援 |
| 介面語言 | 英文、簡體中文、繁體中文、日文 | 與 macOS 相同 |
| 管理員權限 | 僅選用的 Night Shift／原彩排除功能需要 | 安裝程式與 App 皆需要 |

[macOS 詳情](../../macos/README.md) ·
[Windows 相容性與設定](../../WINDOWS.md) ·
[macOS 更新紀錄](../../CHANGELOG.md) ·
[Windows 更新紀錄](../../windows/CHANGELOG.md)

<p align="center">
  <img src="../../docs/en/app-main-v2-1.png" alt="E-Ink Assistant v2.1 英文版" width="440">
</p>

## 控制項

| 控制項 | 適用情境 | 作用 |
|---|---|---|
| 文字對比度 | 閱讀 | 以中等、強、銳利與實邊等級加深淡色文字。較強的等級會犧牲灰階細節，換取更硬的邊緣。 |
| 影片暗部增強 | 照片與影片 | 以輕微、中等與強等級呈現暗部細節。閱讀時請關閉，因為它也會讓深色文字變淡。 |
| 彩度與 RGB | 彩色電子紙 | 在平台支援時，提供六種彩度預設、0%–300% 彩度滑桿，以及 0%–200% RGB 校正。 |
| 減少閃爍 | 支援的 macOS 顯示器 | 停止可見的抖動閃爍，並為標記為電子紙的顯示器自動開啟。 |
| Night Shift 與原彩 | 受色溫變化影響的顯示器 | 將選取的 macOS 顯示器排除於這些功能之外。需要管理員核准並重新連接顯示器，結束 App 後仍保留此設定。 |
| 降低透明度與動態效果 | 更新較慢的面板 | 簡化系統視覺效果。macOS 使用需要使用者確認安裝一次的輔助捷徑。 |
| 進階曲線 | 各面板的專屬調校 | 調整轉折點、Gamma、黑點與白點，提供即時曲線圖與五組具名稱的預設。 |

<p align="center">
  <img src="../../docs/en/text-contrast-editorial.png" alt="文字對比度調整前後的示意圖" width="31%">
  <img src="../../docs/en/video-enhance-editorial.png" alt="影片暗部增強前後的示意圖" width="31%">
  <img src="../../docs/en/saturation-editorial.png" alt="彩度調整前後的示意圖" width="31%">
</p>

> 這些圖片用於示範控制項的效果。實際結果取決於面板與原始素材。

## 安裝

### macOS 14+，Apple silicon

1. 使用上方連結下載 macOS 2.6 DMG。
2. 開啟檔案，並將 **E-Ink Assistant** 拖入**應用程式**。
3. 先嘗試開啟 App 一次。如果 macOS 阻擋開啟，請前往**系統設定 → 隱私權與安全性**，
   然後選擇**強制打開**。

這是獨立開發的軟體，目前未上架 App Store。首次開啟時，macOS 會顯示「無法驗證」警告。
程式碼完全開放，你可以先檢視，再決定是否使用。

如果將 App 移至「應用程式」後未出現**強制打開**，請開啟「終端機」並執行：

```
xattr -dr com.apple.quarantine "/Applications/E-Ink Assistant.app"
```

### Windows 7 SP1 至 Windows 11，x64

1. 使用上方連結下載 Windows 1.2 安裝程式。
2. 執行安裝程式，並核准管理員權限提示。
3. 從「開始」功能表或系統匣開啟 E-Ink Assistant。

請參閱 [WINDOWS.md](../../WINDOWS.md)，了解各項功能依 Windows 版本、GPU、
驅動程式與顯示器連線方式而定的確切支援情況。

## 使用方式

<p align="center">
  <img src="../../docs/en/app-displays-v2-1.png" alt="在 E-Ink Assistant v2.1 中標記顯示器" width="440">
</p>

1. 從 macOS 選單列或 Windows 系統匣開啟 App。
2. 標記每一台你想調校的黑白或彩色電子紙顯示器。
3. 先在顯示器內建選單中設定均衡的硬體對比度。
4. 閱讀時選擇「文字對比度」，觀看媒體時選擇「影片暗部增強」，兩者不可同時使用。
5. 在彩色電子紙上，若平台支援，即可調整彩度與 RGB。

**結束時會還原顯示調整**，啟動時則重新套用。開啟**登入時啟動**即可自動啟動。

## 顯示器設定

在調整 App 之前，請先於顯示器內建選單中設定均衡的對比度。隨附預設是在
**Bigme B251 Pro**（R2 FW V2.0）上，以**網頁模式、硬體 Gamma 等級 3、
對比度 50、關閉色彩還原模式**進行目視調校。黑白面板或其他彩色機型需要各自適合的數值。
進階模式可調整完整曲線，每台顯示器的設定會分開儲存。

「減少閃爍」僅支援 Apple Silicon，在不支援的環境中會自動隱藏。

<details>
<summary>macOS 降低透明度與動態效果輔助捷徑</summary>

首次使用時，需要在 Apple 的「捷徑」App 中確認**加入捷徑**。隨附的輔助捷徑只接受
完全相符的文字指令 `on` 與 `off`，不提供輸出，也不會出現在分享表單、Spotlight、
快速動作或鎖定畫面介面中。它可在 Mac 鎖定時執行。本 App 不會列出或檢查你的其他捷徑。

自動模式會在已標記的電子紙顯示器連接時開啟兩項設定，並在最後一台已標記顯示器中斷連接後
關閉。結束 App 時也會關閉這兩項設定。

</details>

## 專案文件

- [CHANGELOG.md](../../CHANGELOG.md)：各版本的變更
- [TECHNICAL.md](../../TECHNICAL.md)：實作、測量結果，以及在現代 macOS 上*無法*使用的方法
- [mac-saturation](https://github.com/kiteretsu903/mac-saturation)：色彩機制研究與描述檔匯出的命令列工具

## 授權與致謝

採用 MIT 授權，詳見 [LICENSE](../../LICENSE)。

**「減少閃爍」基於 Abdullah Arif 開發的 [Stillcolor](https://github.com/aiaf/Stillcolor)**
（MIT 授權）。Stillcolor 發現可透過 I/O Registry 的 `enableDither` 屬性停用顯示器抖動。
本專案以各顯示器獨立控制的方式重新實作這個概念；這項發現的功勞屬於 Stillcolor。謹此致謝。

完整聲明請見 [THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md)。
