(() => {
  const picker = document.querySelector('#site-language');
  if (!picker) return;
  const supported = [...picker.options].map(option => option.value);
  const current = document.documentElement.lang;
  const base = new URL(document.body.dataset.siteRoot || './', window.location.href);
  const page = document.body.dataset.page === 'changelog' ? 'changelog.html' : '';
  const preferenceKey = 'eink-assistant-language';
  const resolve = input => {
    const value = String(input || '').replaceAll('_', '-').toLowerCase();
    const exact = supported.find(code => code.toLowerCase() === value);
    if (exact) return exact;
    if (/^zh-[a-z]{4}(-|$)/.test(value) && !/^zh-(hans|hant)(-|$)/.test(value)) return undefined;
    if (/^zh-hans(-|$)/.test(value)) return 'zh-Hans';
    if (/^zh-hant(-|$)/.test(value)) return 'zh-Hant';
    if (/^zh-(tw|hk|mo)(-|$)/.test(value)) return 'zh-Hant';
    if (/^zh(-|$)/.test(value)) return 'zh-Hans';
    const parts = value.split('-');
    const base = ({no:'nb', tl:'fil', iw:'he', in:'id'})[parts[0]] || parts[0];
    const script = /^[a-z]{4}$/.test(parts[1] || '') ? parts[1] : undefined;
    if (script) {
      const scripted = supported.find(code => code.toLowerCase() === `${base}-${script}`);
      if (scripted) return scripted;
      try {
        if (new Intl.Locale(base).maximize().script.toLowerCase() !== script) return undefined;
      } catch { return undefined; }
    }
    const region = parts[script ? 2 : 1];
    if (/^(?:[a-z]{2}|[0-9]{3})$/.test(region || '')) {
      const regional = supported.find(code => code.toLowerCase() === `${base}-${region}`);
      if (regional) return regional;
    }
    return supported.find(code => code.toLowerCase() === base)
      || supported.find(code => code.toLowerCase().startsWith(`${base}-`));
  };
  const navigate = (locale, replace = false, preserveEnglish = false) => {
    const target = new URL((locale === 'en' ? '' : `${locale}/`) + page, base);
    const params = new URLSearchParams(window.location.search);
    params.delete('lang');
    if (locale === 'en' && preserveEnglish) params.set('lang', 'en');
    target.search = params.toString();
    target.hash = window.location.hash;
    if (replace) window.location.replace(target.href);
    else window.location.assign(target.href);
  };
  const params = new URLSearchParams(window.location.search);
  const legacy = params.get('lang');
  let requested = resolve(legacy);
  // Explicit locale paths win. Only an unqualified English root uses preferences.
  if (!params.has('lang') && current === 'en') {
    let saved;
    try { saved = localStorage.getItem(preferenceKey); } catch {}
    requested = resolve(saved) || resolve((navigator.languages || [navigator.language])[0]) || 'en';
  }
  let preferenceSaved = false;
  if (params.has('lang') && requested) {
    try { localStorage.setItem(preferenceKey, requested); preferenceSaved = true; } catch {}
  }
  if (requested && requested !== current) {
    navigate(requested, true);
    return;
  }
  const preserveEnglish = current === 'en' && requested === 'en' && params.has('lang') && !preferenceSaved;
  if (legacy || (!page && window.location.pathname.endsWith('/index.html'))) {
    const clean = new URL(window.location.href);
    if (!preserveEnglish) clean.searchParams.delete('lang');
    if (!page) clean.pathname = clean.pathname.replace(/index\.html$/, '');
    history.replaceState(null, '', clean);
  }
  if (preserveEnglish) {
    // Without storage, retain the explicit choice across reloads and page links.
    document.querySelectorAll('a[href]').forEach(link => {
      const target = new URL(link.getAttribute('href'), window.location.href);
      if (target.origin !== base.origin) return;
      const localPages = ['', 'index.html', 'changelog.html'].map(name => base.pathname + name);
      if (!localPages.includes(target.pathname)) return;
      target.searchParams.set('lang', 'en');
      link.setAttribute('href', target.pathname + target.search + target.hash);
    });
  }
  picker.value = current;
  picker.addEventListener('change', () => {
    let saved = false;
    try { localStorage.setItem(preferenceKey, picker.value); saved = true; } catch {}
    navigate(picker.value, false, !saved);
  });
})();
