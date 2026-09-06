(() => {
  const tabs = [...document.querySelectorAll("[data-changelog-platform]")];
  const panels = [...document.querySelectorAll("[data-changelog-panel]")];
  if (!tabs.length || !panels.length) return;

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
      const forward = document.documentElement.dir === 'rtl' ? event.key === 'ArrowLeft' : event.key === 'ArrowRight';
      const next = forward ? (index + 1) % tabs.length : (index - 1 + tabs.length) % tabs.length;
      setPlatform(tabs[next].dataset.changelogPlatform, true);
    });
  });

  const initial = new URLSearchParams(window.location.search).get("platform") === "windows" ? "windows" : "macos";
  setPlatform(initial);
})();
