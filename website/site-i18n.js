/* Shared Czech/English presentation. Recorded inputs, outputs and raw files stay intact. */
(() => {
  'use strict';
  const script = document.currentScript;
  const root = new URL('./', script.src);
  const valid = value => value === 'cs' || value === 'en';
  const route = document.documentElement.dataset.sitePage || decodeURIComponent(location.pathname).slice(root.pathname.length) || 'index.html';
  const guide = route.startsWith('guide/') && route.endsWith('.html');
  const query = new URLSearchParams(location.search).get('lang');
  let saved; try { saved = localStorage.getItem('framelab-language'); } catch {}
  let language = valid(query) ? query : guide ? (route.endsWith('-en.html') || route === 'guide/en.html' ? 'en' : 'cs') : valid(saved) ? saved : 'en';
  let ui = {}, notes = {}, loaded = false, manual = valid(query) || guide || valid(saved);
  const api = window.FrameLabLocale = {
    get language() { return language; },
    t: (cs, en) => language === 'cs' ? cs : en,
    note(text, source = 'cs') { return notes[source + ':' + text]?.[language] ?? text; },
    set: setLanguage,
    ready: null,
  };
  function counterpart(path, lang) {
    if (!path.startsWith('guide/') || !path.endsWith('.html')) return path;
    if (path === 'guide/index.html' || path === 'guide/en.html') return lang === 'cs' ? 'guide/index.html' : 'guide/en.html';
    return path.replace(/(?:-en)?\.html$/, lang === 'cs' ? '.html' : '-en.html');
  }
  function links() {
    document.querySelectorAll('a[href]').forEach(a => {
      if (a.hasAttribute('download') || a.dataset.localeSwitch || a.getAttribute('href').startsWith('#')) return;
      const url = new URL(a.getAttribute('href'), location.href);
      if (url.origin !== root.origin || !url.pathname.startsWith(root.pathname)) return;
      const path = url.pathname.slice(root.pathname.length);
      if (!(path.endsWith('/') || path.endsWith('.html') || path === '')) return;
      url.pathname = root.pathname + counterpart(path, language);
      url.searchParams.set('lang', language); a.href = url.href;
    });
  }
  function apply() {
    document.documentElement.lang = language;
    document.querySelectorAll('[data-en][data-cs]').forEach(e => { const v = e.dataset[language]; if(e.textContent !== v)e.textContent = v; });
    document.querySelectorAll('[data-i18n]').forEach(e => { const v = ui[e.dataset.i18n]?.[language]; if(v !== undefined && e.textContent !== v)e.textContent = v; });
    for (const attr of ['alt','title','placeholder','aria-label','content']) document.querySelectorAll('[data-i18n-'+attr+']').forEach(e => {
      const v = ui[e.getAttribute('data-i18n-'+attr)]?.[language]; if(v !== undefined)e.setAttribute(attr,v);
    });
    document.querySelectorAll('[data-language-only]').forEach(e => { e.hidden = e.dataset.languageOnly !== language; });
    const b = document.querySelector('[data-site-language]');
    if(b) { b.textContent = language === 'cs' ? 'English' : 'Čeština'; b.lang = language === 'cs' ? 'en' : 'cs'; b.setAttribute('aria-label', language === 'cs' ? 'Switch the website to English' : 'Přepnout web do češtiny'); }
    links();
  }
  function setLanguage(lang, persist = true) {
    if(!valid(lang))return;
    language = lang;
    if(persist) { manual = true; try { localStorage.setItem('framelab-language',lang); } catch {} }
    const target = counterpart(route, lang);
    if(guide && target !== route) { const u = new URL(target,root);u.searchParams.set('lang',lang);u.hash=location.hash;location.assign(u);return; }
    const url = new URL(location.href); url.searchParams.set('lang',lang); history.replaceState(null,'',url);
    document.documentElement.lang = lang;
    window.dispatchEvent(new CustomEvent('framelab:language', { detail: { language: lang } }));
    apply();
  }
  let button = document.getElementById('language') || document.getElementById('lang');
  if(!button) {
    const nav = document.createElement('nav'); nav.className = 'site-language-bar'; nav.setAttribute('aria-label','Language / Jazyk');
    const home = document.createElement('a');home.href=new URL('./',root);home.textContent='← FrameLab';
    button=document.createElement('button');button.type='button';nav.append(home,button);document.body.prepend(nav);
  }
  button.dataset.siteLanguage = '';
  // Capture replaces the independent legacy toggles without changing other page controls.
  button.addEventListener('click', e => {e.preventDefault();e.stopImmediatePropagation();setLanguage(language === 'cs' ? 'en' : 'cs');}, true);
  document.querySelectorAll('[data-locale-switch]').forEach(a=>a.addEventListener('click',e=>{e.preventDefault();setLanguage(a.dataset.localeSwitch);}));
  async function json(path) {
    for(let attempt=0;attempt<3;attempt++) {
      try { const r=await fetch(new URL(path,root),{signal:AbortSignal.timeout(8000)});if(!r.ok)throw Error(path);return await r.json(); }
      catch(error) { if(attempt===2)throw error; }
    }
  }
  async function loadNotes() {
    if(!/^(runs\/|translation\/quality)/.test(route))return {};
    const index=await json('i18n/notes-index.json'),files=index[route]||[],data={};
    for(let i=0;i<files.length;i+=3)for(const chunk of await Promise.all(files.slice(i,i+3).map(json)))Object.assign(data,chunk);
    return data;
  }
  api.ready = Promise.all([document.documentElement.dataset.uiCatalog ? json(document.documentElement.dataset.uiCatalog) : Promise.resolve({}),loadNotes()]).then(([u,n]) => {
    ui=u;notes=n;loaded=true;setLanguage(language,false);document.documentElement.dataset.localeReady='true';
    if(!manual)fetch(new URL('locale.php',root),{cache:'no-store',signal:AbortSignal.timeout(2000)}).then(r=>r.json()).then(r=>{if(!manual)setLanguage(valid(r.language)?r.language:'en',false);}).catch(()=>{});
  }).catch(() => {
    document.documentElement.dataset.localeReady='error';
    const error=document.createElement('p');error.className='site-locale-error';error.textContent='Language files could not load. Reload this page. / Jazykové soubory se nenačetly. Obnov stránku.';button.after(error);
  });
  // Late data rendering can create new labelled UI and links; never translate arbitrary text.
  let scheduled=false;
  new MutationObserver(records => {
    if(!loaded || scheduled || !records.some(r=>r.addedNodes.length && [...r.addedNodes].some(n=>n.nodeType===1)))return;
    scheduled=true;queueMicrotask(()=>{scheduled=false;apply();});
  }).observe(document.body,{childList:true,subtree:true});
})();
