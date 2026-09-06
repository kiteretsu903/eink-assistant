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
    const original = copyButton.dataset.copyLabel || copyButton.textContent;
    const copied = copyButton.dataset.copiedLabel;
    const failed = copyButton.dataset.copyFailedLabel;
    try {
      await navigator.clipboard.writeText(command);
      copyButton.textContent = copied;
      window.setTimeout(() => { copyButton.textContent = original; }, 1600);
    } catch {
      copyButton.textContent = failed;
      window.setTimeout(() => { copyButton.textContent = original; }, 3500);
    }
  });
})();
