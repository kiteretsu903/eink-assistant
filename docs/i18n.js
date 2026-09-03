(() => {
  const copy = {
    "zh-Hans": {
      lang: "zh-Hans",
      title: "E-Ink Assistant：让墨水屏在 macOS 上更好用",
      description: "E-Ink Assistant 为 macOS 上的黑白与彩色墨水屏提供专属调校。",
      nav: ["功能", "使用方法", "更新日志"],
      getApp: "获取应用 <span aria-hidden=\"true\">↗</span>",
      home: {
        eyebrow: "<span></span> 专为 macOS 与 Apple 芯片打造",
        hero: "让墨水屏在 macOS 上<br><em>更稳定、更清晰。</em>",
        lede: "E-Ink Assistant 是一款轻巧的菜单栏应用，可改善黑白与彩色墨水屏的稳定性、文字显示和色彩，同时不影响 Mac 内建显示屏。",
        download: "下载 macOS 版 <span aria-hidden=\"true\">↓</span>", star: "喜欢的话，欢迎在 GitHub 点亮 Star。", latest: "最新版本", releaseDate: "2026 年 9 月 2 日", fine: "macOS 14+ · Apple Silicon · 免费开源", tag: "你的墨水屏<br>显示器",
        ticker: "显示更稳定 <b>✦</b> 文字更清晰 <b>✦</b> 暗部更易辨认 <b>✦</b> 色彩更自然 <b>✦</b> 显示更稳定 <b>✦</b> 文字更清晰 <b>✦</b> 暗部更易辨认 <b>✦</b> 色彩更自然 <b>✦</b>",
        introNo: "01 / 专为 macOS 设计", intro: "专为 macOS 与墨水屏<br><em>打造的显示工具。</em>", introP: ["E-Ink Assistant 是一款适用于 Apple 芯片 Mac 和 macOS 14+ 的菜单栏应用。它只调节你标记为墨水屏的外接显示器，Mac 内建显示屏和其他显示器不受影响。", "核心显示控制无需任何权限，也不会安装独立的后台服务。"],
        presets: ["使用内置基线", "针对 Bigme B251 Pro 的 R2 固件 V2.0 优化。显示器应设为网页模式、硬件 Gamma 3 级、对比度 50，并关闭 Color Restore Mode。", "其他黑白与彩色墨水屏", "先设置均衡的硬件基线", "先为面板设置均衡的硬件参数，再用高级调校微调并保存曲线，可获得更稳定的软件调校效果。"],
        mac: ["专为 macOS 打造", "一个菜单栏应用。<br>每台显示器独立控制。", "适用于搭载 Apple 芯片的 macOS 14+，支持黑白与彩色墨水屏。", "无需核心权限", "显示控制无需辅助功能权限。", "不影响其他显示器", "每台已标记的墨水屏都保存独立配置。", "自动还原", "退出时还原设置，启动时重新应用。"],
        groups: [["01 / 显示稳定性", "让墨水屏显示更稳定", "<strong>减少闪烁与动态效果</strong>可抑制可见的抖动，并简化不适合低刷新率屏幕的 macOS 动画。", "<strong>显示器跟随</strong>会在已标记的显示器连接时自动应用所选配置。"], ["02 / 阅读与媒体", "分别优化文字与媒体", "<strong>文字对比度</strong>会加深偏浅的文字。提供中、强、锐利和纯黑四档。", "<strong>视频暗部增强</strong>可还原照片和视频中的暗部细节。提供轻微、中等和强三档。"], ["03 / 彩色墨水屏", "彩色墨水屏调校", "<strong>饱和度与 RGB</strong>提供 6 种预设、0%–300% 滑杆，以及独立的 0%–200% RGB 通道校正。", "<strong>每台显示器独立的 Night Shift 与 True Tone 控制</strong>只关闭墨水屏上的 macOS 色温处理。"], ["04 / 高级调校", "曲线与预设", "<strong>完整色调曲线</strong>可配合实时图表调节拐点、伽马、黑点和白点。", "<strong>5 个可命名的预设槽位与快捷指令</strong>可保存配置并自动控制显示器行为。"]],
        compare: ["02 / 实际效果", "细微调整。<br><em>阅读体验大不同。</em>", "应用会针对不同内容进行色调调整：加深需要阅读的文字，只提亮掩盖画面细节的暗部，或补偿窄色域面板的色彩。", "文字对比度", "加深偏浅的正文与次要文字，让页面更易阅读。", "视频暗部增强", "提亮照片和视频中的暗部，呈现更多细节。", "色彩饱和度", "让彩色墨水屏呈现更鲜明、更易辨认的色彩。", "功能效果示意图。实际效果会因面板和素材而异。"],
        workflow: ["03 / 简单易用", "一个应用，三步完成。<br><em>其他屏幕不受影响。</em>", "连接显示器", "像平常一样连接黑白或彩色墨水屏。", "标记为墨水屏", "在 E-Ink Assistant 中选择要调校的显示器，其他屏幕不受影响。", "选择当前用途", "阅读时加深文字，看视频时提亮暗部，也可以微调已保存的配置。"],
        advanced: ["04 / 需要更多时", "你的面板，<br><em>精确到曲线。</em>", "借助实时曲线微调拐点、伽马、黑点和白点。可为不同显示器、房间或使用情境保存五个命名预设。", "配置按显示器保存。退出时恢复调整，启动时重新应用。"],
        bottom: ["让墨水屏更好用", "让墨水屏<br><em>真正融入 macOS。</em>", "免费、尊重隐私，专为你每天使用的墨水屏打造。", "下载 E-Ink Assistant <span aria-hidden=\"true\">↓</span>", "macOS 14+ · Apple Silicon · MIT 许可证"], footer: ["© 2026 E‑Ink Assistant", "在 GitHub 查看源代码 <span aria-hidden=\"true\">↗</span>"]
      },
      changelog: { intro: ["更新记录", "改了什么，<br><em>何时改的。</em>", "从最新版本到首个版本，查看 E‑Ink Assistant 的更新。"], releases: [["版本 2.5", "2026 年 9 月 2 日", "新增简洁的硬件设置提示。", "控制面板顶部新增可关闭的提示，并提供 Bigme B251 Pro 示例。", "“知道了”仅在本次运行期间隐藏提示；“不再显示”会在重新启动后继续隐藏。", "欢迎面板改为三个简洁、带图标的设置要点。", "显示器选择列表现在先显示外接显示器，内建显示屏排在最后。"], ["版本 2.4", "2026 年 9 月 1 日", "控制面板高度按当前屏幕调整。", "控制面板现在使用菜单栏图标所在屏幕的可见高度。", "内容过长时会滚动，并与程序坞或屏幕底边保持 40 点间距；内容较少时使用自然高度。"], ["版本 2.3", "2026 年 9 月 1 日", "调整文字对比度预设值。", "“中等”使用之前的“强”曲线；“强”“锐利”和“实边”逐档加深。", "“锐利”使用 Gamma 5.00 和 0.10 黑点；“实边”使用 Gamma 6.00 和 0.34 黑点。两者会损失更多灰阶细节，边缘也更硬。"], ["版本 2.2", "2026 年 8 月 14 日", "修复“工作原理”的语言切换。", "更改应用语言后，区段标题和“展开 / 收起”按钮会立即更新。"], ["版本 2.1", "2026 年 8 月 14 日", "更新设置辅助指令。", "辅助指令可在 Mac 锁定时运行，避免断开显示器或清理设置时出现“快捷指令”警告。", "从 v2.0 升级的用户需要添加一次改名后的辅助指令，之后可移除旧指令。"], ["版本 2.0", "2026 年 8 月 13 日", "新增显示控制、语言和 DMG 安装器。", "新增逐显示器 RGB、Night Shift 和 True Tone 控制。", "新增通过用户确认的辅助指令控制“降低透明度”和动态效果。", "新增英语、简体中文、繁体中文和日语。"], ["版本 1.1", "", "新增逐显示器 Night Shift 和 True Tone 控制。", "此更改需要管理员密码并重新连接显示器；退出应用时不会还原。"], ["版本 1.0", "", "首个版本。", "新增减少抖动、饱和度、文字对比度、视频暗部增强、自定义色调曲线、已保存曲线和退出时还原显示设置。"]], github: ["开源", "喜欢这个项目吗？", "在 GitHub 给 E‑Ink Assistant 一个 Star"] }
    },
    "zh-Hant": {
      lang: "zh-Hant",
      title: "E-Ink Assistant：讓電子紙在 macOS 上更好用",
      description: "E-Ink Assistant 為 macOS 上的黑白與彩色電子紙顯示器提供專屬調校。",
      nav: ["功能", "使用方式", "更新日誌"],
      getApp: "取得應用程式 <span aria-hidden=\"true\">↗</span>",
      home: {
        eyebrow: "<span></span> 專為 macOS 與 Apple 晶片打造", hero: "讓電子紙在 macOS 上<br><em>更穩定、更清晰。</em>", lede: "E-Ink Assistant 是一款輕巧的選單列 App，可改善黑白與彩色電子紙的穩定性、文字顯示與色彩，同時不影響 Mac 內建螢幕。", download: "下載 macOS 版 <span aria-hidden=\"true\">↓</span>", star: "喜歡的話，歡迎在 GitHub 給個 Star。", latest: "最新版本", releaseDate: "2026 年 9 月 2 日", fine: "macOS 14+ · Apple Silicon · 免費開源", tag: "你的電子紙<br>顯示器",
        ticker: "顯示更穩定 <b>✦</b> 文字更清晰 <b>✦</b> 暗部更易辨識 <b>✦</b> 色彩更自然 <b>✦</b> 顯示更穩定 <b>✦</b> 文字更清晰 <b>✦</b> 暗部更易辨識 <b>✦</b> 色彩更自然 <b>✦</b>",
        introNo: "01 / 為 macOS 設計", intro: "專為 macOS 與電子紙<br><em>打造的顯示工具。</em>", introP: ["E-Ink Assistant 在搭載 Apple 晶片的 macOS 14+ 上作為選單列應用程式執行。它只調整你標記為電子紙的外接顯示器，因此 Mac 內建螢幕與其他顯示器保持原樣。", "核心顯示控制不需要任何權限，也不會安裝獨立的背景服務。"],
        presets: ["使用內建基線", "針對 Bigme B251 Pro 的 R2 韌體 V2.0 最佳化。顯示器應設為網頁模式、硬體 Gamma 等級 3、對比度 50，並關閉 Color Restore Mode。", "其他黑白與彩色電子紙", "先設定均衡的硬體基準", "先為面板設定均衡的硬體參數，再用進階調校微調並儲存曲線，可獲得更穩定的軟體調校效果。"],
        mac: ["專為 macOS 打造", "一個選單列 App。<br>每台顯示器獨立控制。", "適用於搭載 Apple 晶片的 macOS 14+，支援黑白與彩色電子紙。", "不需核心權限", "顯示控制不需輔助使用權限。", "不影響其他顯示器", "每台已標記的電子紙都會儲存獨立設定。", "自動還原", "結束時還原設定，啟動時重新套用。"],
        groups: [["01 / 顯示穩定性", "讓電子紙顯示更穩定", "<strong>減少閃爍與動態效果</strong>可抑制可見的抖動，並簡化不適合低更新率螢幕的 macOS 動畫。", "<strong>顯示器跟隨</strong>會在已標記的顯示器連接時自動套用所選設定。"], ["02 / 閱讀與媒體", "分別最佳化文字與媒體", "<strong>文字對比度</strong>會加深偏淡的文字。提供中、強、銳利與純黑四檔。", "<strong>影片暗部增強</strong>可還原照片與影片中的暗部細節。提供輕微、中等與強三檔。"], ["03 / 彩色電子紙", "彩色電子紙調校", "<strong>彩度與 RGB</strong>提供 6 種預設、0%–300% 滑桿，以及獨立的 0%–200% RGB 色彩通道校正。", "<strong>每台顯示器獨立的 Night Shift 與 True Tone 控制</strong>只會關閉電子紙上的 macOS 色溫處理。"], ["04 / 進階調校", "曲線與預設", "<strong>完整色調曲線</strong>可配合即時圖表調整轉折點、Gamma、黑點與白點。", "<strong>5 個可命名的預設槽位與捷徑</strong>可儲存設定並自動控制顯示器行為。"]],
        compare: ["02 / 實際效果", "細微調整。<br><em>閱讀體驗大不同。</em>", "App 會依內容進行色調調整：加深需要閱讀的文字，只提亮掩蓋畫面細節的暗部，或補償窄色域面板的色彩。", "文字對比度", "加深偏淡的本文與次要文字，讓頁面更容易閱讀。", "影片暗部增強", "提亮照片與影片中的暗部，呈現更多細節。", "色彩飽和度", "讓彩色電子紙呈現更鮮明、更容易辨識的色彩。", "功能效果示意圖。實際效果會因面板與素材而異。"],
        workflow: ["03 / 簡單易用", "一個 App，三個步驟。<br><em>其他螢幕不受影響。</em>", "連接顯示器", "像平常一樣連接黑白或彩色電子紙。", "標記為電子紙", "在 E-Ink Assistant 中選擇要調校的顯示器，其他螢幕不受影響。", "選擇目前用途", "閱讀時加深文字，看影片時提亮暗部，也可以微調已儲存的設定。"],
        advanced: ["04 / 需要更多時", "你的面板，<br><em>精確到曲線。</em>", "藉助即時曲線微調轉折點、伽瑪、黑點和白點。可為不同顯示器、房間或使用情境儲存五個命名預設。", "設定按顯示器儲存。結束時還原調整，啟動時重新套用。"],
        bottom: ["讓電子紙更好用", "讓電子紙<br><em>真正融入 macOS。</em>", "免費、尊重隱私，專為你每天使用的電子紙打造。", "下載 E-Ink Assistant <span aria-hidden=\"true\">↓</span>", "macOS 14+ · Apple Silicon · MIT 授權條款"], footer: ["© 2026 E‑Ink Assistant", "在 GitHub 查看原始碼 <span aria-hidden=\"true\">↗</span>"]
      },
      changelog: { intro: ["更新日誌", "改了什麼，<br><em>何時改的。</em>", "從最新版本到首個版本，查看 E‑Ink Assistant 的更新。"], releases: [["版本 2.5", "2026 年 9 月 2 日", "新增簡潔的硬體設定提示。", "控制面板頂端新增可關閉的提示，並提供 Bigme B251 Pro 範例。", "「知道了」只在本次執行期間隱藏提示；「不要再顯示」會在重新啟動後繼續隱藏。", "歡迎面板改為三個簡潔、附圖示的設定要點。", "顯示器選擇清單現在先顯示外接顯示器，內建顯示器排在最後。"], ["版本 2.4", "2026 年 9 月 1 日", "控制面板高度依目前螢幕調整。", "控制面板現在使用選單列圖示所在螢幕的可見高度。", "內容過長時會捲動，並與 Dock 或螢幕底邊保持 40 點間距；內容較少時使用自然高度。"], ["版本 2.3", "2026 年 9 月 1 日", "調整文字對比度預設值。", "「中等」使用先前的「強」曲線；「強」「銳利」與「實邊」逐級加深。", "「銳利」使用 Gamma 5.00 與 0.10 黑點；「實邊」使用 Gamma 6.00 與 0.34 黑點。兩者會損失更多灰階細節，邊緣也較硬。"], ["版本 2.2", "2026 年 8 月 14 日", "修正「運作方式」的語言切換。", "變更 App 語言後，區段標題與「顯示更多 / 顯示較少」按鈕會立即更新。"], ["版本 2.1", "2026 年 8 月 14 日", "更新設定輔助捷徑。", "輔助捷徑可在 Mac 鎖定時執行，避免中斷顯示器或清理設定時出現捷徑警告。", "從 v2.0 升級的使用者需要加入一次改名後的輔助捷徑，之後可移除舊捷徑。"], ["版本 2.0", "2026 年 8 月 13 日", "新增顯示控制、語言與 DMG 安裝程式。", "新增逐顯示器 RGB、Night Shift 與 True Tone 控制。", "新增透過使用者確認的輔助捷徑控制降低透明度與動態效果。", "新增英語、簡體中文、繁體中文與日語。"], ["版本 1.1", "", "新增逐顯示器 Night Shift 與 True Tone 控制。", "此變更需要管理員密碼並重新連接顯示器；結束 App 時不會還原。"], ["版本 1.0", "", "首個版本。", "新增減少抖動、彩度、文字對比度、影片暗部增強、自訂色調曲線、已儲存曲線與結束時還原顯示設定。"]], github: ["開源", "喜歡這個專案嗎？", "在 GitHub 給 E‑Ink Assistant 一個 Star"] }
    },
    ja: {
      lang: "ja",
      title: "E-Ink Assistant：電子ペーパーを macOS でもっと快適に",
      description: "E-Ink Assistant は、macOS のモノクロ・カラー電子ペーパーディスプレイを調整します。",
      nav: ["機能", "使い方", "更新履歴"], getApp: "アプリを入手 <span aria-hidden=\"true\">↗</span>",
      home: {
        eyebrow: "<span></span> macOS と Apple シリコンのために", hero: "電子ペーパーを、<br><em>もっと快適に。</em>", lede: "E-Ink Assistant は、Mac の内蔵ディスプレイに影響を与えず、モノクロとカラーの電子ペーパーディスプレイのちらつきや表示を整える軽量なメニューバーアプリです。", download: "macOS 版をダウンロード <span aria-hidden=\"true\">↓</span>", star: "気に入ったら、GitHub で Star をお願いします。", latest: "最新リリース", releaseDate: "2026年9月2日", fine: "macOS 14+ · Apple Silicon · 無料・オープンソース", tag: "電子ペーパー<br>ディスプレイ",
        ticker: "安定した表示 <b>✦</b> 読みやすい文字 <b>✦</b> 見やすい暗部 <b>✦</b> 自然な色 <b>✦</b> 安定した表示 <b>✦</b> 読みやすい文字 <b>✦</b> 見やすい暗部 <b>✦</b> 自然な色 <b>✦</b>",
        introNo: "01 / macOS のための設計", intro: "電子ペーパーを<br><em>macOS で快適に。</em>", introP: ["E-Ink Assistant は、Apple シリコン搭載 Mac の macOS 14 以降で動作するメニューバーアプリです。電子ペーパーとして登録した外部ディスプレイだけを調整し、Mac の内蔵ディスプレイや他のモニターには影響しません。", "主要な表示調整に権限は不要で、別のバックグラウンドサービスもインストールしません。"],
        presets: ["内蔵設定を使う", "Bigme B251 Pro の R2 FW V2.0 向けに最適化されています。本体を Web Mode、Hardware Gamma Level 3、Contrast 50、Color Restore Mode オフに設定してください。", "その他のモノクロ・カラー電子ペーパー", "まず本体をバランスよく設定", "まずパネル本体をバランスよく設定し、詳細調整でカーブを微調整して保存すると、安定した結果を得やすくなります。"],
        mac: ["macOS のために", "1つのメニューバーアプリ。<br>ディスプレイごとに調整。", "Apple シリコン搭載 Mac の macOS 14 以降に対応。モノクロとカラーの電子ペーパーで使えます。", "主要な権限は不要", "表示調整にアクセシビリティ権限は不要です。", "他のディスプレイに影響しない", "登録した電子ペーパーパネルごとに設定を保存します。", "自動で元に戻す", "終了時に変更を戻し、起動時に再適用します。"],
        groups: [["01 / 表示の安定性", "電子ペーパーの表示を安定させる", "<strong>ちらつきと動きを抑える機能</strong>で、目立つディザリングを軽減し、更新の遅いパネルに不向きな macOS のアニメーションを減らします。", "<strong>ディスプレイへの自動追従</strong>で、登録したディスプレイの接続時に選んだ設定を適用します。"], ["02 / 読書とメディア", "文字と映像を別々に調整", "<strong>テキストコントラスト</strong>は薄い文字を濃くします。中、強、シャープ、ソリッドの4段階です。", "<strong>映像の暗部補正</strong>は写真や動画の暗部を見やすくします。弱、中、強の3段階です。"], ["03 / カラー電子ペーパー", "カラー電子ペーパーを調整", "<strong>彩度と RGB</strong>では、6つのプリセット、0%–300% のスライダー、各色を独立して調整できる 0%–200% の RGB 補正を利用できます。", "<strong>ディスプレイごとの Night Shift と True Tone 制御</strong>は、選んだ電子ペーパーパネルだけで macOS の色温度処理を無効にします。"], ["04 / 詳細調整", "カーブとプリセット", "<strong>トーンカーブ全体</strong>を見ながら、ニー、ガンマ、黒点、白点を調整できます。", "<strong>5つの名前付きプリセットとショートカット</strong>で設定を保存し、ディスプレイの動作を自動化できます。"]],
        compare: ["02 / 実際の効果", "小さな調整で、<br><em>もっと読みやすく。</em>", "読む文字は濃く、写真や動画の暗部は見やすく、狭い色域のパネルでは色を鮮明にします。内容に合わせて必要な部分だけを調整します。", "テキストコントラスト", "薄い本文や補助テキストを濃くし、ページを読みやすくします。", "映像の暗部補正", "写真や動画の暗部を明るくし、つぶれた細部を見やすくします。", "彩度補正", "カラー電子ペーパーで色をより鮮明に、見分けやすくします。", "機能を説明するイメージです。実際の効果はパネルや素材によって異なります。"],
        workflow: ["03 / シンプルな使い方", "3ステップで調整。<br><em>他の画面はそのまま。</em>", "ディスプレイを接続", "いつもどおり、モノクロまたはカラーの電子ペーパーディスプレイを接続します。", "電子ペーパーとして登録", "E-Ink Assistant で調整するディスプレイを選びます。他の画面には影響しません。", "用途別に調整", "読書では文字を濃くし、動画では暗部を明るくします。保存済みの設定を微調整することもできます。"],
        advanced: ["04 / 細かく調整する", "パネルごとに、<br><em>カーブまで細かく。</em>", "カーブを見ながら、ニー、ガンマ、黒点、白点を微調整できます。ディスプレイ、部屋、用途に合わせて、名前付きの設定を5つまで保存できます。", "設定はディスプレイごとに保存されます。終了時に調整を戻し、起動時に再適用します。"],
        bottom: ["電子ペーパーをもっと快適に", "電子ペーパーを<br><em>もっと快適に。</em>", "無料で、プライバシーを尊重した、毎日使う電子ペーパーのためのアプリです。", "E-Ink Assistant をダウンロード <span aria-hidden=\"true\">↓</span>", "macOS 14+ · Apple Silicon · MIT ライセンス"], footer: ["© 2026 E‑Ink Assistant", "GitHub でソースを見る <span aria-hidden=\"true\">↗</span>"]
      },
      changelog: { intro: ["更新履歴", "何が変わったか、<br><em>いつ変わったか。</em>", "最新バージョンから初回リリースまで、E‑Ink Assistant の更新を確認できます。"], releases: [["バージョン 2.5", "2026年9月2日", "簡潔な本体設定ガイドを追加。", "コントロール上部に、Bigme B251 Pro の例を含む閉じられるガイドを追加しました。", "「わかりました」は今回の起動中だけ非表示にし、「今後表示しない」は再起動後も非表示にします。", "ようこそパネルを、簡潔な3つのアイコン付き設定ポイントに変更しました。", "ディスプレイ選択では外付けディスプレイを先に、内蔵ディスプレイを最後に表示します。"], ["バージョン 2.4", "2026年9月1日", "パネルの高さを現在の画面に合わせるよう変更。", "コントロールパネルは、メニューバーアイコンがある画面の使用可能な高さに合わせます。", "内容が長い場合はスクロールし、Dock または画面下端との間に 40 ポイントの余白を残します。内容が短い場合は自然な高さのままです。"], ["バージョン 2.3", "2026年9月1日", "テキストコントラストの設定値を変更。", "「中」は以前の「強」のカーブを使用し、「強」「シャープ」「ソリッド」の順に濃くなります。", "「シャープ」は Gamma 5.00、黒点 0.10。「ソリッド」は Gamma 6.00、黒点 0.34。どちらも階調が減り、縁が硬くなります。"], ["バージョン 2.2", "2026年8月14日", "「仕組み」の言語切り替えを修正。", "アプリの言語を変更すると、見出しと「詳細を表示 / 閉じる」ボタンがすぐに更新されます。"], ["バージョン 2.1", "2026年8月14日", "設定用ショートカットを更新。", "Mac がロック中でも実行でき、ディスプレイ切断時や終了処理時のショートカット警告を防ぎます。", "v2.0 から更新する場合は、名前を変更したショートカットを一度追加し、以前のものを削除できます。"], ["バージョン 2.0", "2026年8月13日", "表示制御、言語、DMG インストーラを追加。", "ディスプレイごとの RGB、Night Shift、True Tone 制御を追加しました。", "ユーザー確認済みのショートカットで透明度と視差効果を下げる機能を追加しました。", "英語、簡体字中国語、繁体字中国語、日本語を追加しました。"], ["バージョン 1.1", "", "ディスプレイごとの Night Shift と True Tone 制御を追加。", "管理者パスワードとディスプレイの再接続が必要です。アプリ終了時には元に戻りません。"], ["バージョン 1.0", "", "初回リリース。", "ちらつきの軽減、彩度、テキストコントラスト、映像の暗部補正、カスタムトーンカーブ、保存したカーブ、終了時の表示復元を追加しました。"]], github: ["オープンソース", "気に入りましたか？", "GitHub で E‑Ink Assistant に Star を付ける"] }
    }
  };

  const supported = ["en", "zh-Hans", "zh-Hant", "ja"];
  const select = (selector, value, html = false) => { const element = document.querySelector(selector); if (element && value !== undefined) element[html ? "innerHTML" : "textContent"] = value; };
  const selectAll = (selector, values, html = false) => document.querySelectorAll(selector).forEach((element, index) => { if (values[index] !== undefined) element[html ? "innerHTML" : "textContent"] = values[index]; });

  function localeFrom(value) {
    const lang = String(value || "").toLowerCase();
    if (lang.startsWith("ja")) return "ja";
    if (lang.startsWith("zh-hant") || lang.startsWith("zh-tw") || lang.startsWith("zh-hk") || lang.startsWith("zh-mo")) return "zh-Hant";
    if (lang.startsWith("zh")) return "zh-Hans";
    return "en";
  }

  function applyHome(d) {
    const h = d.home;
    select(".eyebrow", h.eyebrow, true); select(".hero-copy h1", h.hero, true); select(".lede", h.lede); select(".hero-actions .button", h.download, true); select(".hero-star span", h.star); select(".hero-release > span", h.latest); select(".hero-release time", h.releaseDate); select(".hero-copy > .fine-print", h.fine); select(".display-tag", h.tag, true); select(".ticker-track span", h.ticker, true);
    select("#features > .section-number", h.introNo); select(".intro-grid h2", h.intro, true); selectAll(".intro-grid > div > p", h.introP);
    selectAll(".preset-path .preset-eyebrow", ["BIGME B251 PRO", h.presets[2]]); selectAll(".preset-path h3", [h.presets[0], h.presets[3]]); selectAll(".preset-path p:last-child", [h.presets[1], h.presets[4]]);
    select(".catalogue-kicker", h.mac[0]); select(".macos-card h3", h.mac[1], true); select(".macos-card > p:not(.catalogue-kicker)", h.mac[2]); selectAll(".macos-card dt", [h.mac[3], h.mac[5], h.mac[7]]); selectAll(".macos-card dd", [h.mac[4], h.mac[6], h.mac[8]]);
    h.groups.forEach((group, index) => { const n = index + 1; select(`.feature-group:nth-child(${n}) > p`, group[0]); select(`.feature-group:nth-child(${n}) h3`, group[1]); selectAll(`.feature-group:nth-child(${n}) li`, [group[2], group[3]], true); });
    select(".comparison .section-number", h.compare[0]); select(".improvements-head h2", h.compare[1], true); select(".improvements-head > p", h.compare[2]); selectAll(".gallery-card strong", [h.compare[3], h.compare[5], h.compare[7]]); selectAll(".gallery-card figcaption span", [h.compare[4], h.compare[6], h.compare[8]]); select(".gallery-note", h.compare[9]);
    select(".workflow .section-number", h.workflow[0]); select(".workflow h2", h.workflow[1], true); selectAll(".workflow .steps h3", [h.workflow[2], h.workflow[4], h.workflow[6]]); selectAll(".workflow .steps p", [h.workflow[3], h.workflow[5], h.workflow[7]]);
    select(".advanced .section-number", h.advanced[0]); select(".advanced h2", h.advanced[1], true); selectAll(".advanced-copy > p:not(.section-number)", [h.advanced[2], h.advanced[3]]);
    select(".download .section-number", h.bottom[0]); select(".download h2", h.bottom[1], true); select(".download-content > p:not(.section-number):not(.fine-print)", h.bottom[2]); select(".download .button", h.bottom[3], true); select(".download .fine-print", h.bottom[4]); selectAll(".site-footer > *", h.footer, true);
  }

  function applyChangelog(d) {
    const c = d.changelog;
    select(".changelog-intro .section-number", c.intro[0]); select(".changelog-intro h1", c.intro[1], true); select(".changelog-intro > p:last-child", c.intro[2]);
    c.releases.forEach((release, index) => { const n = index + 1; select(`.release-list article:nth-child(${n}) .release-version p`, release[0]); if (release[1]) select(`.release-list article:nth-child(${n}) time`, release[1]); select(`.release-list article:nth-child(${n}) h2`, release[2]); selectAll(`.release-list article:nth-child(${n}) li`, release.slice(3), true); });
    select(".changelog-github p", c.github[0]); select(".changelog-github h2", c.github[1]); select(".changelog-github .github-star span", c.github[2]); selectAll(".site-footer > *", d.home.footer, true);
  }

  function updateInternalLinks(locale) {
    document.querySelectorAll("a[href]").forEach((link) => {
      const href = link.getAttribute("href");
      if (!href || !/^(index\.html|changelog\.html)(#.*)?$/.test(href)) return;
      if (locale === "en") { link.setAttribute("href", href); return; }
      const [path, hash = ""] = href.split("#");
      link.setAttribute("href", `${path}?lang=${encodeURIComponent(locale)}${hash ? `#${hash}` : ""}`);
    });
  }

  function apply(locale) {
    const d = copy[locale];
    document.documentElement.lang = d.lang;
    document.title = d.title;
    const description = document.querySelector('meta[name="description"]'); if (description) description.content = d.description;
    selectAll("nav a", d.nav); select(".button-small", d.getApp, true);
    const heroImage = document.querySelector(".hero-visual .app-window");
    if (heroImage) {
      heroImage.src = `${locale}/app-main-v2-1.png`;
      heroImage.alt = {
        "zh-Hans": "E-Ink Assistant 简体中文主界面",
        "zh-Hant": "E-Ink Assistant 繁體中文主介面",
        ja: "E-Ink Assistant 日本語メイン画面"
      }[locale];
    }
    const advancedImage = document.querySelector(".advanced-image img");
    if (advancedImage) {
      advancedImage.src = `${locale}/app-advanced-v2-1.png`;
      advancedImage.alt = {
        "zh-Hans": "E-Ink Assistant 简体中文高级曲线调校界面",
        "zh-Hant": "E-Ink Assistant 繁體中文進階曲線調校介面",
        ja: "E-Ink Assistant 日本語の高度なカーブ調整画面"
      }[locale];
    }
    if (document.body.dataset.page === "home") applyHome(d); else applyChangelog(d);
    updateInternalLinks(locale);
    const canonical = document.querySelector('link[rel="canonical"]');
    if (canonical) { const base = document.body.dataset.page === "home" ? "https://kiteretsu903.github.io/eink-assistant/" : "https://kiteretsu903.github.io/eink-assistant/changelog.html"; canonical.href = locale === "en" ? base : `${base}?lang=${encodeURIComponent(locale)}`; }
  }

  const params = new URLSearchParams(window.location.search);
  let resetAfterLanguageSwitch = false;
  try {
    resetAfterLanguageSwitch = sessionStorage.getItem("eink-assistant-language-switch") === "1";
    if (resetAfterLanguageSwitch) sessionStorage.removeItem("eink-assistant-language-switch");
  } catch {}
  if (resetAfterLanguageSwitch && "scrollRestoration" in history) history.scrollRestoration = "manual";
  let storedLocale = "";
  try { storedLocale = localStorage.getItem("eink-assistant-language") || ""; } catch {}
  let locale = params.has("lang") ? localeFrom(params.get("lang")) : (supported.includes(storedLocale) ? storedLocale : "");
  if (!locale) locale = localeFrom((navigator.languages || [navigator.language]).find((language) => localeFrom(language) !== "en") || "en");
  if (!supported.includes(locale)) locale = "en";
  if (locale !== "en") apply(locale);
  if (resetAfterLanguageSwitch) {
    const resetScroll = () => window.scrollTo(0, 0);
    resetScroll();
    requestAnimationFrame(resetScroll);
    window.addEventListener("pageshow", resetScroll, { once: true });
  }
  const picker = document.querySelector("#site-language");
  if (picker) { picker.value = locale; picker.addEventListener("change", () => { const next = picker.value; try { localStorage.setItem("eink-assistant-language", next); sessionStorage.setItem("eink-assistant-language-switch", "1"); } catch {} const url = new URL(window.location.href); if (next === "en") url.searchParams.delete("lang"); else url.searchParams.set("lang", next); url.hash = ""; window.location.assign(url); }); }
})();
