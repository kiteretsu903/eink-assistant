(() => {
  const translations = {
    "zh-Hans": {
      title: "E-Ink Assistant：适用于 macOS 与 Windows 的墨水屏调校工具",
      description: "E-Ink Assistant 可在 macOS 与 Windows 上调节墨水屏的文字对比度、暗部细节和色彩。",
      nav: ["用途", "功能", "平台对比", "更新日志"], headerDownload: "下载 <span aria-hidden=\"true\">↓</span>",
      hero: ["<span></span> macOS 与 Windows", "根据用途<br><em>调好墨水屏显示器。</em>", "调节所选墨水屏显示器的文字对比度、暗部细节和色彩，其他显示器保持不变。", "macOS 2.6", "Windows 1.2", "macOS 14+ · Apple Silicon / Windows 7–11 · x64", "你的墨水屏<br>显示器", "文字对比度 <b>✦</b> 暗部细节 <b>✦</b> 饱和度与 RGB <b>✦</b> 每台显示器独立预设 <b>✦</b> 文字对比度 <b>✦</b> 暗部细节 <b>✦</b> 饱和度与 RGB <b>✦</b> 每台显示器独立预设 <b>✦</b>", "GitHub + Star"],
      matrix: {
        title: "功能与系统要求", columns: ["功能", "macOS 2.6", "Windows 1.2"],
        rows: [["支持的系统", "<strong>macOS 14 或更高版本</strong><br>仅支持 Apple 芯片", "<strong>Windows 7 SP1 至 Windows 11</strong><br>x64 电脑"], ["应用所在位置", "菜单栏", "系统托盘"], ["选择特定墨水屏", "支持，其他显示器保持不变。", "同 macOS"], ["文字对比度", "中、强、锐利、纯黑四档", "同 macOS"], ["视频暗部增强", "轻微、中等、强三档", "同 macOS"], ["高级曲线与预设", "实时曲线编辑器和五个命名预设", "同 macOS"], ["饱和度与 RGB", "逐显示器色彩配置；饱和度 0%–300%，RGB 0%–200%", "适用的 Windows 10 2004 和 Windows 11 21H2+；具体可用方式取决于系统与硬件。"], ["减少抖动", "在支持的外接显示器上可用；标记为墨水屏时自动开启", "不可用。Windows 没有统一的公开逐显示器抖动控制，也大概率不需要。"], ["降低透明度与动态效果", "通过一次用户确认的辅助指令启用", "Windows 7 SP1 起通过兼容的系统 API 提供"], ["系统浅色模式", "不更改", "Windows 10 1903+ 可在本次会话中启用 Windows 浅色模式"], ["Night Shift / 夜间模式", "逐显示器排除 Night Shift 与原彩显示；需要管理员确认并重新连接显示器", "Windows 10 1703+ 可打开夜间模式设置；Windows 11 24H2+ 提供直接关闭控制。"], ["镜像 / 复制显示", "镜像中的物理显示器仍可分别选择", "色调曲线会影响共用信号源；饱和度与 RGB 需要使用扩展模式"], ["退出时恢复", "退出时恢复临时曲线、色彩配置和抖动；Night Shift / 原彩显示排除会保留", "退出时恢复临时 Gamma、色彩、视觉与夜间模式；色彩和夜间模式也支持异常退出恢复"], ["登录时启动", "支持", "支持"], ["界面语言", "英语、简体中文、繁体中文、日语", "同 macOS"], ["管理员权限", "仅在选择性排除 Night Shift / 原彩显示时需要", "安装器和应用都需要"]],
        versionTitle: "Windows 功能版本要求", versions: ["<strong>Windows 7 SP1+</strong>：文字对比度、视频暗部增强、高级曲线、预设、降低透明度与动态效果、显示器跟随和登录时启动。", "<strong>Windows 10 1703+</strong>：夜间模式设置入口。", "<strong>Windows 10 1903+</strong>：本次会话使用 Windows 浅色模式。", "<strong>Windows 10 2004–22H2</strong>：兼容系统支持饱和度与 RGB；是否可用取决于 GPU、驱动和显示路径。", "<strong>Windows 11 21H2+</strong>：支持核心功能；色彩控制取决于系统与硬件。Windows 11 24H2+ 还提供直接关闭夜间模式控制。"]
      },
      catalogueLabel: "02 / 功能一览",
      summary: ["MAC 与 WINDOWS", "共有功能，<br>系统限制不同。", "应用仅使用各操作系统提供的公开显示控制。不支持的控制会被隐藏或说明原因。", "两者都有", "文字对比度、视频暗部增强、曲线、预设和逐显示器设置。", "仅 macOS", "减少抖动，以及逐显示器的 Night Shift / 原彩显示处理。", "WINDOWS 色彩", "适用的 Windows 10 2004 和 Windows 11 21H2+；是否可用取决于系统与硬件。"],
      groups: [["01 / 阅读", "加深偏浅的文字", "<strong>文字对比度</strong>提供四档强度，适合低对比度页面。", "阅读时使用；它与视频暗部增强是两个独立功能。"], ["02 / 照片与视频", "显示更多暗部细节", "<strong>视频暗部增强</strong>会提亮照片和视频中的暗部。", "阅读时请关闭，因为它也会提亮深色文字。"], ["03 / 彩色墨水屏", "调节饱和度和 RGB", "用预设或直接控制补偿有限的色域。", "是否可用取决于操作系统、GPU、驱动和显示器连接方式。"], ["04 / 每台显示器", "让设置跟随面板", "只标记你希望应用调节的墨水屏。", "为不同显示器保存曲线，并在退出应用时恢复设置。"]],
      compare: ["01 / 各项控制的用途", "根据内容<br><em>选择控制。</em>", "文字对比度用于阅读页面；视频暗部增强用于深色画面；饱和度和 RGB 用于彩色墨水屏。", "文字对比度", "加深偏浅的正文与次要文字。", "视频暗部增强", "提亮照片和视频中的暗部。", "色彩控制", "用饱和度和 RGB 补偿偏淡的色彩。", "功能效果示意图。实际结果因面板和素材而异。"],
      advanced: ["03 / 可选的精细调校", "预设不够时，<br><em>再调曲线。</em>", "一边查看曲线，一边调整拐点、Gamma、黑点和白点。", "最多保存五个命名预设；每台显示器分别保存设置。"],
      download: ["下载", "让墨水屏，<br><em>成为你的主场。</em>", "两个版本都免费、开源。", "macOS 14+ Apple 芯片 · Windows 7 SP1–11 x64"],
      macDialog: ["在 Mac 上安装", "下载并打开 DMG。", "将 <strong>E‑Ink Assistant</strong> 拖入 <strong>Applications（应用程序）</strong>。", "先尝试打开应用一次。如果 macOS 阻止打开，请前往<strong>系统设置 → 隐私与安全性</strong>，选择<strong>仍要打开</strong>。", "这是个人开发的软件，目前未上架 App Store。首次打开时，macOS 会提示无法验证；代码全部开源，你可以查看后再决定是否使用。", "如果没有出现“仍要打开”", "将应用移入 Applications 后，打开“终端”并运行：", "下载 macOS 2.6 <span aria-hidden=\"true\">↓</span>", "如果对你有帮助，欢迎在 GitHub 点 Star <span aria-hidden=\"true\">↗</span>", "关闭", "复制"],
      windowsDialog: ["在 Windows 上安装", "下载并运行 Setup 安装文件。", "确认管理员权限提示。", "从开始菜单或系统托盘打开 E‑Ink Assistant。", "适用于 x64 电脑上的 Windows 7 SP1 至 Windows 11。可用控制取决于 Windows 版本、GPU、驱动和显示器连接方式。", "下载 Windows 1.2 <span aria-hidden=\"true\">↓</span>", "如果对你有帮助，欢迎在 GitHub 点 Star <span aria-hidden=\"true\">↗</span>", "关闭"],
      footer: ["© 2026 E‑Ink Assistant", "在 GitHub 查看源代码 <span aria-hidden=\"true\">↗</span>"]
    },
    "zh-Hant": {
      title: "E-Ink Assistant：適用於 macOS 與 Windows 的電子紙調校工具",
      description: "E-Ink Assistant 可在 macOS 與 Windows 上調整電子紙的文字對比、暗部細節與色彩。",
      nav: ["用途", "功能", "平台比較", "更新日誌"], headerDownload: "下載 <span aria-hidden=\"true\">↓</span>",
      hero: ["<span></span> macOS 與 Windows", "依照用途<br><em>調好電子紙顯示器。</em>", "調整所選電子紙顯示器的文字對比、暗部細節與色彩，其他顯示器保持不變。", "macOS 2.6", "Windows 1.2", "macOS 14+ · Apple Silicon / Windows 7–11 · x64", "你的電子紙<br>顯示器", "文字對比 <b>✦</b> 暗部細節 <b>✦</b> 彩度與 RGB <b>✦</b> 每台顯示器獨立預設 <b>✦</b> 文字對比 <b>✦</b> 暗部細節 <b>✦</b> 彩度與 RGB <b>✦</b> 每台顯示器獨立預設 <b>✦</b>", "GitHub + Star"],
      matrix: {
        title: "功能與系統需求", columns: ["功能", "macOS 2.6", "Windows 1.2"],
        rows: [["支援的系統", "<strong>macOS 14 或更新版本</strong><br>僅支援 Apple 晶片", "<strong>Windows 7 SP1 至 Windows 11</strong><br>x64 電腦"], ["App 所在位置", "選單列", "系統匣"], ["選擇特定電子紙", "支援，其他顯示器保持不變。", "同 macOS"], ["文字對比", "中、強、銳利、純黑四檔", "同 macOS"], ["影片暗部增強", "輕微、中等、強三檔", "同 macOS"], ["進階曲線與預設", "即時曲線編輯器和五個命名預設", "同 macOS"], ["彩度與 RGB", "逐顯示器色彩描述檔；彩度 0%–300%，RGB 0%–200%", "適用的 Windows 10 2004 與 Windows 11 21H2+；具體可用方式取決於系統與硬體。"], ["減少抖動", "在支援的外接顯示器上可用；標記為電子紙時自動開啟", "不可用。Windows 沒有統一的公開逐顯示器抖動控制，也大多不需要。"], ["降低透明度與動態效果", "透過一次使用者確認的輔助捷徑啟用", "Windows 7 SP1 起透過相容的系統 API 提供"], ["系統淺色模式", "不變更", "Windows 10 1903+ 可在本次工作階段啟用 Windows 淺色模式"], ["Night Shift / 夜間光線", "逐顯示器排除 Night Shift 與 True Tone；需要系統管理員確認並重新連接顯示器", "Windows 10 1703+ 可開啟夜間光線設定；Windows 11 24H2+ 提供直接關閉控制。"], ["鏡像 / 複製顯示", "鏡像中的實體顯示器仍可分別選擇", "色調曲線會影響共用訊號來源；彩度與 RGB 需要使用延伸模式"], ["結束時還原", "結束時還原暫時曲線、色彩描述檔與抖動；Night Shift / True Tone 排除會保留", "結束時還原暫時 Gamma、色彩、視覺與夜間光線；色彩和夜間光線也支援異常結束復原"], ["登入時啟動", "支援", "支援"], ["介面語言", "英語、簡體中文、繁體中文、日語", "同 macOS"], ["系統管理員權限", "僅在選擇性排除 Night Shift / True Tone 時需要", "安裝程式和 App 都需要"]],
        versionTitle: "Windows 功能版本需求", versions: ["<strong>Windows 7 SP1+</strong>：文字對比、影片暗部增強、進階曲線、預設、降低透明度與動態效果、顯示器跟隨和登入時啟動。", "<strong>Windows 10 1703+</strong>：夜間光線設定入口。", "<strong>Windows 10 1903+</strong>：本次工作階段使用 Windows 淺色模式。", "<strong>Windows 10 2004–22H2</strong>：相容系統支援彩度與 RGB；是否可用取決於 GPU、驅動程式與顯示路徑。", "<strong>Windows 11 21H2+</strong>：支援核心功能；色彩控制取決於系統與硬體。Windows 11 24H2+ 另提供直接關閉夜間光線控制。"]
      },
      catalogueLabel: "02 / 功能一覽",
      summary: ["MAC 與 WINDOWS", "共有功能，<br>系統限制不同。", "App 只使用各作業系統提供的公開顯示控制。不支援的控制會隱藏或說明原因。", "兩者都有", "文字對比、影片暗部增強、曲線、預設與逐顯示器設定。", "僅 macOS", "減少抖動，以及逐顯示器的 Night Shift / True Tone 處理。", "WINDOWS 色彩", "適用的 Windows 10 2004 與 Windows 11 21H2+；是否可用取決於系統與硬體。"],
      groups: [["01 / 閱讀", "加深偏淡的文字", "<strong>文字對比</strong>提供四檔強度，適合低對比頁面。", "閱讀時使用；它與影片暗部增強是兩個獨立功能。"], ["02 / 照片與影片", "顯示更多暗部細節", "<strong>影片暗部增強</strong>會提亮照片與影片中的暗部。", "閱讀時請關閉，因為它也會提亮深色文字。"], ["03 / 彩色電子紙", "調整彩度與 RGB", "用預設或直接控制補償有限的色域。", "是否可用取決於作業系統、GPU、驅動程式與顯示器連接方式。"], ["04 / 每台顯示器", "讓設定跟隨面板", "只標記你希望 App 調整的電子紙。", "為不同顯示器儲存曲線，並在結束 App 時還原設定。"]],
      compare: ["01 / 各項控制的用途", "依照內容<br><em>選擇控制。</em>", "文字對比用於閱讀頁面；影片暗部增強用於深色畫面；彩度與 RGB 用於彩色電子紙。", "文字對比", "加深偏淡的本文與次要文字。", "影片暗部增強", "提亮照片與影片中的暗部。", "色彩控制", "用彩度與 RGB 補償偏淡的色彩。", "功能效果示意圖。實際結果會因面板與素材而異。"],
      advanced: ["03 / 選用的精細調校", "預設不足時，<br><em>再調曲線。</em>", "一邊查看曲線，一邊調整轉折點、Gamma、黑點與白點。", "最多儲存五個命名預設；每台顯示器分別儲存設定。"],
      download: ["下載", "讓電子紙，<br><em>成為你的主場。</em>", "兩個版本都免費、開源。", "macOS 14+ Apple 晶片 · Windows 7 SP1–11 x64"],
      macDialog: ["在 Mac 上安裝", "下載並開啟 DMG。", "將 <strong>E‑Ink Assistant</strong> 拖入 <strong>Applications（應用程式）</strong>。", "先嘗試開啟 App 一次。如果 macOS 阻止開啟，請前往<strong>系統設定 → 隱私權與安全性</strong>，選擇<strong>強制打開</strong>。", "這是個人開發的軟體，目前未上架 App Store。第一次開啟時，macOS 會提示無法驗證；程式碼完全開源，你可以查看後再決定是否使用。", "如果沒有出現「強制打開」", "將 App 移入 Applications 後，開啟「終端機」並執行：", "下載 macOS 2.6 <span aria-hidden=\"true\">↓</span>", "如果對你有幫助，歡迎在 GitHub 給個 Star <span aria-hidden=\"true\">↗</span>", "關閉", "複製"],
      windowsDialog: ["在 Windows 上安裝", "下載並執行 Setup 安裝檔。", "確認系統管理員權限提示。", "從開始功能表或系統匣開啟 E‑Ink Assistant。", "適用於 x64 電腦上的 Windows 7 SP1 至 Windows 11。可用控制取決於 Windows 版本、GPU、驅動程式與顯示器連接方式。", "下載 Windows 1.2 <span aria-hidden=\"true\">↓</span>", "如果對你有幫助，歡迎在 GitHub 給個 Star <span aria-hidden=\"true\">↗</span>", "關閉"],
      footer: ["© 2026 E‑Ink Assistant", "在 GitHub 查看原始碼 <span aria-hidden=\"true\">↗</span>"]
    },
    ja: {
      title: "E-Ink Assistant：macOS・Windows 向け電子ペーパーディスプレイ調整ツール",
      description: "E-Ink Assistant は macOS と Windows で電子ペーパーディスプレイの文字、暗部、色を調整します。",
      nav: ["用途", "機能", "環境比較", "更新履歴"], headerDownload: "ダウンロード <span aria-hidden=\"true\">↓</span>",
      hero: ["<span></span> macOS・Windows 対応", "用途に合わせて<br><em>電子ペーパーディスプレイを調整。</em>", "選んだ電子ペーパーディスプレイの文字、暗部、色を調整します。他のディスプレイは変更しません。", "macOS 2.6", "Windows 1.2", "macOS 14+ · Apple Silicon / Windows 7–11 · x64", "電子ペーパー<br>ディスプレイ", "テキストコントラスト <b>✦</b> 暗部補正 <b>✦</b> 彩度 + RGB <b>✦</b> ディスプレイ別プリセット <b>✦</b> テキストコントラスト <b>✦</b> 暗部補正 <b>✦</b> 彩度 + RGB <b>✦</b> ディスプレイ別プリセット <b>✦</b>", "GitHub + Star"],
      matrix: {
        title: "機能とシステム要件", columns: ["機能", "macOS 2.6", "Windows 1.2"],
        rows: [["対応システム", "<strong>macOS 14 以降</strong><br>Apple シリコンのみ", "<strong>Windows 7 SP1 から Windows 11</strong><br>x64 PC"], ["アプリの場所", "メニューバー", "システムトレイ"], ["特定の電子ペーパーを選択", "対応。他のディスプレイは変更しません。", "macOS と同じ"], ["テキストコントラスト", "中、強、シャープ、ソリッドの4段階", "macOS と同じ"], ["映像暗部補正", "弱、中、強の3段階", "macOS と同じ"], ["詳細カーブとプリセット", "カーブ表示付きエディターと5つの名前付きプリセット", "macOS と同じ"], ["彩度と RGB", "ディスプレイ別カラープロファイル。彩度 0%–300%、RGB 0%–200%", "Windows 10 2004 および Windows 11 21H2+ の対応環境で利用できます。利用方法はシステムとハードウェアによって異なります。"], ["ちらつき軽減", "対応する外部ディスプレイで利用可能。電子ペーパーに登録すると自動で有効化", "非対応です。Windows には統一された公開ディスプレイ別ディザリング制御がなく、多くの場合は必要ありません。"], ["透明度と視差効果を減らす", "一度ユーザーが確認する補助ショートカットで利用可能", "Windows 7 SP1 から互換性のあるシステム API で利用可能"], ["システムのライトモード", "変更しません", "Windows 10 1903+ でセッション中のみ Windows ライトモードを使用"], ["Night Shift / 夜間モード", "ディスプレイ別に Night Shift と True Tone を除外。管理者の承認と再接続が必要", "Windows 10 1703+ は夜間モード設定を開けます。Windows 11 24H2+ は直接無効化できます。"], ["ミラー / 複製表示", "ミラー内の物理ディスプレイも個別に選択可能", "トーンカーブは共有ソースに作用。彩度と RGB には拡張モードが必要"], ["終了時の復元", "一時的なカーブ、カラープロファイル、ディザリングを終了時に復元。Night Shift / True Tone 除外は保持", "一時的なガンマ、カラー、視覚設定、夜間モードを終了時に復元。カラーと夜間モードは異常終了後も復旧"], ["ログイン時に起動", "対応", "対応"], ["表示言語", "英語、簡体字中国語、繁体字中国語、日本語", "macOS と同じ"], ["管理者権限", "Night Shift / True Tone 除外を使う場合のみ必要", "インストーラーとアプリで必要"]],
        versionTitle: "Windows 機能別の要件", versions: ["<strong>Windows 7 SP1+</strong>：テキストコントラスト、映像暗部補正、詳細カーブ、プリセット、透明度と視差効果の低減、ディスプレイ追従、ログイン時起動。", "<strong>Windows 10 1703+</strong>：夜間モード設定へのリンク。", "<strong>Windows 10 1903+</strong>：セッション中のみ Windows ライトモード。", "<strong>Windows 10 2004–22H2</strong>：対応環境で彩度と RGB を利用できます。利用可否は GPU、ドライバー、表示経路によって異なります。", "<strong>Windows 11 21H2+</strong>：基本機能に対応。カラー制御はシステムとハードウェアによって異なります。Windows 11 24H2+ では夜間モードを直接無効化できます。"]
      },
      catalogueLabel: "02 / 機能一覧",
      summary: ["MAC と WINDOWS", "共通機能と、<br>OS ごとの制限。", "各 OS が公開しているディスプレイ制御だけを使用します。非対応の項目は非表示にするか理由を示します。", "共通", "テキストコントラスト、映像暗部補正、カーブ、プリセット、ディスプレイ別設定。", "macOS のみ", "ちらつき軽減と、ディスプレイ別 Night Shift / True Tone 制御。", "WINDOWS の色調整", "Windows 10 2004 および Windows 11 21H2+ の対応環境。利用可否はシステムとハードウェアによって異なります。"],
      groups: [["01 / 読書", "薄い文字を濃くする", "<strong>テキストコントラスト</strong>は低コントラストのページ向けに4段階あります。", "読書用です。映像暗部補正とは別に使います。"], ["02 / 写真と動画", "暗部の情報を見やすくする", "<strong>映像暗部補正</strong>は写真や動画の暗い部分を明るくします。", "濃い文字も明るくなるため、読書時はオフにします。"], ["03 / カラー電子ペーパー", "彩度と RGB を調整", "プリセットまたは直接調整で狭い色域を補います。", "利用可否は OS、GPU、ドライバー、接続方式によって異なります。"], ["04 / ディスプレイ別", "パネルごとに設定を保存", "アプリで変更する電子ペーパーディスプレイだけを登録します。", "ディスプレイごとにカーブを保存し、終了時に設定を元に戻します。"]],
      compare: ["01 / 各機能の用途", "内容に合う<br><em>機能を選ぶ。</em>", "テキストコントラストは読書、映像暗部補正は暗い画像、彩度と RGB はカラー電子ペーパーに使います。", "テキストコントラスト", "薄い本文や補助文字を濃くします。", "映像暗部補正", "写真や動画の暗部を明るくします。", "カラー調整", "彩度と RGB で淡い色を補います。", "機能のイメージです。結果はパネルと素材によって異なります。"],
      advanced: ["03 / 必要に応じて微調整", "プリセットで足りないときは、<br><em>カーブを調整。</em>", "カーブを見ながらニー、ガンマ、黒点、白点を変更します。", "最大5つの名前付きプリセットを、ディスプレイごとに保存できます。"],
      download: ["ダウンロード", "電子ペーパーを、<br><em>もっと自分らしく。</em>", "どちらも無料・オープンソースです。", "macOS 14+ Apple シリコン · Windows 7 SP1–11 x64"],
      macDialog: ["Mac にインストール", "DMG をダウンロードして開きます。", "<strong>E‑Ink Assistant</strong> を <strong>Applications</strong> にドラッグします。", "一度アプリを開きます。macOS に止められた場合は、<strong>システム設定 → プライバシーとセキュリティ</strong>で<strong>このまま開く</strong>を選びます。", "個人開発のソフトウェアで、現在 App Store では配布していません。初回起動時に macOS が「検証できません」と表示します。ソースコードはすべて公開されているため、確認してから使用するか判断できます。", "「このまま開く」が表示されない場合", "Applications に移動した後、ターミナルを開いて次を実行します：", "macOS 2.6 をダウンロード <span aria-hidden=\"true\">↓</span>", "役に立ったら GitHub で Star をお願いします <span aria-hidden=\"true\">↗</span>", "閉じる", "コピー"],
      windowsDialog: ["Windows にインストール", "Setup ファイルをダウンロードして実行します。", "管理者権限の確認を許可します。", "スタートメニューまたはシステムトレイから E‑Ink Assistant を開きます。", "x64 の Windows 7 SP1 から Windows 11 に対応。利用できる機能は Windows のバージョン、GPU、ドライバー、接続方式によって異なります。", "Windows 1.2 をダウンロード <span aria-hidden=\"true\">↓</span>", "役に立ったら GitHub で Star をお願いします <span aria-hidden=\"true\">↗</span>", "閉じる"],
      footer: ["© 2026 E‑Ink Assistant", "GitHub でソースを見る <span aria-hidden=\"true\">↗</span>"]
    }
  };

  const comparisonRefinements = {
    "zh-Hans": {
      columns: ["功能", "macOS", "Windows"], same: "同 macOS",
      color: "适用的 Windows 10 2004 和 Windows 11 21H2+；具体可用方式取决于系统与硬件。",
      shaking: "不可用。Windows 没有统一的公开逐显示器抖动控制，也大概率不需要。",
      night: "Windows 10 1703+ 可打开夜间模式设置；Windows 11 24H2+ 提供直接关闭控制。",
      version10: "<strong>Windows 10 2004–22H2</strong>：兼容系统支持饱和度与 RGB；是否可用取决于 GPU、驱动和显示路径。",
      version11: "<strong>Windows 11 21H2+</strong>：支持核心功能；色彩控制取决于系统与硬件。Windows 11 24H2+ 还提供直接关闭夜间模式控制。",
      summary: "适用的 Windows 10 2004 和 Windows 11 21H2+；是否可用取决于系统与硬件。"
    },
    "zh-Hant": {
      columns: ["功能", "macOS", "Windows"], same: "同 macOS",
      color: "適用的 Windows 10 2004 與 Windows 11 21H2+；具體可用方式取決於系統與硬體。",
      shaking: "不可用。Windows 沒有統一的公開逐顯示器抖動控制，也大多不需要。",
      night: "Windows 10 1703+ 可開啟夜間光線設定；Windows 11 24H2+ 提供直接關閉控制。",
      version10: "<strong>Windows 10 2004–22H2</strong>：相容系統支援彩度與 RGB；是否可用取決於 GPU、驅動程式與顯示路徑。",
      version11: "<strong>Windows 11 21H2+</strong>：支援核心功能；色彩控制取決於系統與硬體。Windows 11 24H2+ 另提供直接關閉夜間光線控制。",
      summary: "適用的 Windows 10 2004 與 Windows 11 21H2+；是否可用取決於系統與硬體。"
    },
    ja: {
      columns: ["機能", "macOS", "Windows"], same: "macOS と同じ",
      color: "Windows 10 2004 および Windows 11 21H2+ の対応環境で利用できます。利用方法はシステムとハードウェアによって異なります。",
      shaking: "非対応です。Windows には統一された公開ディスプレイ別ディザリング制御がなく、多くの場合は必要ありません。",
      night: "Windows 10 1703+ は夜間モード設定を開けます。Windows 11 24H2+ は直接無効化できます。",
      version10: "<strong>Windows 10 2004–22H2</strong>：対応環境で彩度と RGB を利用できます。利用可否は GPU、ドライバー、表示経路によって異なります。",
      version11: "<strong>Windows 11 21H2+</strong>：基本機能に対応。カラー制御はシステムとハードウェアによって異なります。Windows 11 24H2+ では夜間モードを直接無効化できます。",
      summary: "Windows 10 2004 および Windows 11 21H2+ の対応環境。利用可否はシステムとハードウェアによって異なります。"
    }
  };

  Object.entries(comparisonRefinements).forEach(([key, refinement]) => {
    const localized = translations[key];
    localized.matrix.columns = refinement.columns;
    [2, 3, 4, 5, 14].forEach((row) => { localized.matrix.rows[row][2] = refinement.same; });
    localized.matrix.rows[6][2] = refinement.color;
    localized.matrix.rows[7][2] = refinement.shaking;
    localized.matrix.rows[10][2] = refinement.night;
    localized.matrix.rows[13][2] = key === "ja" ? "対応" : (key === "zh-Hant" ? "支援" : "支持");
    localized.matrix.versions[3] = refinement.version10;
    localized.matrix.versions[4] = refinement.version11;
    localized.summary[8] = refinement.summary;
  });

  const locale = document.documentElement.lang;
  const d = translations[locale];
  if (!d || document.body.dataset.page !== "home") return;

  const set = (selector, value, html = false) => {
    const element = document.querySelector(selector);
    if (element && value !== undefined) element[html ? "innerHTML" : "textContent"] = value;
  };
  const setAll = (selector, values, html = false) => document.querySelectorAll(selector).forEach((element, index) => {
    if (values[index] !== undefined) element[html ? "innerHTML" : "textContent"] = values[index];
  });

  document.title = d.title;
  document.querySelector('meta[name="description"]').content = d.description;
  setAll("nav a", d.nav);
  set(".site-header .button-small", d.headerDownload, true);

  set(".eyebrow", d.hero[0], true); set(".hero-copy h1", d.hero[1], true); set(".lede", d.hero[2]);
  set('.platform-actions [data-download-platform="mac"] span:nth-of-type(1)', d.hero[3]); set('.platform-actions [data-download-platform="windows"] span:nth-of-type(1)', d.hero[4]);
  set(".hero-copy > .fine-print", d.hero[5]); set(".display-tag", d.hero[6], true); set(".ticker-track span", d.hero[7], true); set(".github-star-label", d.hero[8]);

  set(".comparison-title h3", d.matrix.title); set(".feature-comparison thead th:first-child", d.matrix.columns[0]); set(".feature-comparison thead th:nth-child(2) .platform-column-title span", d.matrix.columns[1]); set(".feature-comparison thead th:nth-child(3) .platform-column-title span", d.matrix.columns[2]);
  d.matrix.rows.forEach((row, index) => { const selector = `.feature-comparison tbody tr:nth-child(${index + 1})`; set(`${selector} th`, row[0]); setAll(`${selector} td`, row.slice(1), true); });
  document.querySelector(".comparison-table-wrap").ariaLabel = `${d.matrix.columns[1]} / ${d.matrix.columns[2]} ${d.matrix.title}`;

  set(".feature-catalogue > .catalogue-section-number", d.catalogueLabel); set(".platform-summary .catalogue-kicker", d.summary[0]); set(".platform-summary h3", d.summary[1], true); set(".platform-summary > p:not(.catalogue-kicker)", d.summary[2]);
  setAll(".platform-summary dt", [d.summary[3], d.summary[5], d.summary[7]]); setAll(".platform-summary dd", [d.summary[4], d.summary[6], d.summary[8]]);
  d.groups.forEach((group, index) => { const selector = `.feature-group:nth-child(${index + 1})`; set(`${selector} > p`, group[0]); set(`${selector} h3`, group[1]); setAll(`${selector} li`, group.slice(2), true); });

  set(".comparison .section-number", d.compare[0]); set(".improvements-head h2", d.compare[1], true); set(".improvements-head > p", d.compare[2]);
  setAll(".gallery-card strong", [d.compare[3], d.compare[5], d.compare[7]]); setAll(".gallery-card figcaption span", [d.compare[4], d.compare[6], d.compare[8]]); set(".gallery-note", d.compare[9]);
  set(".advanced .section-number", d.advanced[0]); set(".advanced h2", d.advanced[1], true); setAll(".advanced-copy > p:not(.section-number)", d.advanced.slice(2));
  set(".download .section-number", d.download[0]); set(".download h2", d.download[1], true); set(".download-content > p:not(.section-number):not(.fine-print)", d.download[2]); set(".download .fine-print", d.download[3]);

  set("#mac-dialog-title", d.macDialog[0]); setAll("#mac-download-dialog .install-steps li", d.macDialog.slice(1, 4), true); set("#mac-download-dialog .dialog-note", d.macDialog[4]); set("#mac-download-dialog summary", d.macDialog[5]); set("#mac-download-dialog details p", d.macDialog[6]); set("#mac-download-dialog .dialog-actions .button", d.macDialog[7], true); set("#mac-download-dialog .dialog-star", d.macDialog[8], true); document.querySelector("#mac-download-dialog .dialog-close").ariaLabel = d.macDialog[9]; set("[data-copy-command]", d.macDialog[10]);
  set("#windows-dialog-title", d.windowsDialog[0]); setAll("#windows-download-dialog .install-steps li", d.windowsDialog.slice(1, 4)); set("#windows-download-dialog .dialog-note", d.windowsDialog[4]); set("#windows-download-dialog .dialog-actions .button", d.windowsDialog[5], true); set("#windows-download-dialog .dialog-star", d.windowsDialog[6], true); document.querySelector("#windows-download-dialog .dialog-close").ariaLabel = d.windowsDialog[7];
  setAll(".site-footer > *", d.footer, true);
})();
