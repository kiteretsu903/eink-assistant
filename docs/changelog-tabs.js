(() => {
  const tabs = [...document.querySelectorAll("[data-changelog-platform]")];
  const panels = [...document.querySelectorAll("[data-changelog-panel]")];
  if (!tabs.length || !panels.length) return;

  const translations = {
    "zh-Hans": {
      label: "选择更新日志平台",
      status: "当前版本",
      releases: [
        ["版本 1.2", "改进色彩控制恢复。", "修复 Windows 提供色彩通道、但不允许应用更改其状态时出现的错误失败提示。", "复制模式暂时停用色彩控制时会保留恢复数据；回到可控制的显示模式后再完成恢复。"],
        ["版本 1.1", "Windows 首个版本。", "提供逐显示器的文字对比度、视频暗部增强、高级曲线、预设和墨水屏选择。", "在兼容系统上提供饱和度、RGB、Windows 浅色模式和夜间模式控制，并支持退出还原与异常恢复。", "提供复制显示处理、系统托盘引导、登录时启动、更新和多语言安装器。"]
      ]
    },
    "zh-Hant": {
      label: "選擇更新日誌平台",
      status: "目前版本",
      releases: [
        ["版本 1.2", "改進色彩控制復原。", "修正 Windows 提供色彩通道、但不允許 App 變更其狀態時出現的錯誤失敗提示。", "鏡像模式暫時停用色彩控制時會保留復原資料；回到可控制的顯示模式後再完成復原。"],
        ["版本 1.1", "Windows 首個版本。", "提供逐顯示器的文字對比、影片暗部增強、進階曲線、預設和電子紙選擇。", "在相容系統上提供彩度、RGB、Windows 淺色模式和夜間光線控制，並支援結束還原與異常復原。", "提供鏡像顯示處理、系統匣引導、登入時啟動、更新和多語言安裝程式。"]
      ]
    },
    ja: {
      label: "更新履歴のプラットフォームを選択",
      status: "現行バージョン",
      releases: [
        ["バージョン 1.2", "カラー制御の復元を改善。", "Windows にカラー経路があってもアプリによる状態変更が許可されない場合に、誤った失敗表示が出る問題を修正しました。", "複製表示でカラー制御が一時的に無効になる間は復元データを保持し、制御可能な表示構成に戻ってから復元します。"],
        ["バージョン 1.1", "Windows 版の初回リリース。", "ディスプレイ別のテキストコントラスト、映像暗部補正、詳細カーブ、プリセット、電子ペーパーディスプレイ選択を追加しました。", "対応環境で彩度、RGB、Windows ライトモード、夜間モードを制御し、終了時の復元と異常終了後の回復に対応しました。", "複製表示、システムトレイ案内、ログイン時起動、更新、多言語インストーラーに対応しました。"]
      ]
    }
  };

  const setPlatform = (platform, focus = false) => {
    tabs.forEach((tab) => {
      const selected = tab.dataset.changelogPlatform === platform;
      tab.classList.toggle("is-active", selected);
      tab.setAttribute("aria-selected", String(selected));
      tab.tabIndex = selected ? 0 : -1;
      if (selected && focus) tab.focus();
    });
    panels.forEach((panel) => { panel.hidden = panel.dataset.changelogPanel !== platform; });
    const url = new URL(window.location.href);
    if (platform === "windows") url.searchParams.set("platform", "windows");
    else url.searchParams.delete("platform");
    history.replaceState(null, "", url);
  };

  tabs.forEach((tab, index) => {
    tab.addEventListener("click", () => setPlatform(tab.dataset.changelogPlatform));
    tab.addEventListener("keydown", (event) => {
      if (!['ArrowLeft', 'ArrowRight'].includes(event.key)) return;
      event.preventDefault();
      const next = event.key === 'ArrowRight' ? (index + 1) % tabs.length : (index - 1 + tabs.length) % tabs.length;
      setPlatform(tabs[next].dataset.changelogPlatform, true);
    });
  });

  const localized = translations[document.documentElement.lang];
  if (localized) {
    document.querySelector(".platform-tabs")?.setAttribute("aria-label", localized.label);
    const status = document.querySelector("#windows-changelog .release-status");
    if (status) status.textContent = localized.status;
    localized.releases.forEach((release, index) => {
      const article = document.querySelector(`#windows-changelog article:nth-child(${index + 1})`);
      if (!article) return;
      const version = article.querySelector(".release-version p");
      const title = article.querySelector("h2");
      if (version) version.textContent = release[0];
      if (title) title.textContent = release[1];
      article.querySelectorAll("li").forEach((item, itemIndex) => {
        if (release[itemIndex + 2]) item.textContent = release[itemIndex + 2];
      });
    });
  }

  const initial = new URLSearchParams(window.location.search).get("platform") === "windows" ? "windows" : "macos";
  setPlatform(initial);
})();
