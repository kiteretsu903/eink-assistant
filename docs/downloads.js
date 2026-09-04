(() => {
  let returnFocus = null;

  function dialogFor(platform) {
    return document.querySelector(`#${platform}-download-dialog`);
  }

  document.querySelectorAll("[data-download-platform]").forEach((trigger) => {
    trigger.addEventListener("click", (event) => {
      const dialog = dialogFor(trigger.dataset.downloadPlatform);
      if (!dialog || typeof dialog.showModal !== "function") return;
      event.preventDefault();
      returnFocus = trigger;
      dialog.showModal();
      dialog.querySelector("[data-dialog-close]")?.focus();
    });
  });

  document.querySelectorAll(".download-dialog").forEach((dialog) => {
    dialog.querySelector("[data-dialog-close]")?.addEventListener("click", () => dialog.close());
    dialog.addEventListener("click", (event) => {
      if (event.target === dialog) dialog.close();
    });
    dialog.addEventListener("close", () => {
      returnFocus?.focus();
      returnFocus = null;
    });
  });

  const copyButton = document.querySelector("[data-copy-command]");
  copyButton?.addEventListener("click", async () => {
    const command = copyButton.previousElementSibling?.textContent || "";
    const labels = {
      en: ["Copy", "Copied"],
      "zh-Hans": ["复制", "已复制"],
      "zh-Hant": ["複製", "已複製"],
      ja: ["コピー", "コピー済み"]
    };
    const locale = document.documentElement.lang;
    try {
      await navigator.clipboard.writeText(command);
      copyButton.textContent = (labels[locale] || labels.en)[1];
      window.setTimeout(() => { copyButton.textContent = (labels[locale] || labels.en)[0]; }, 1600);
    } catch {
      copyButton.textContent = (labels[locale] || labels.en)[0];
    }
  });
})();
