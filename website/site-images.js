// Original screenshots: fit to viewport or inspect at one source pixel per CSS pixel.
(() => {
  const t = (cs,en) => document.documentElement.lang === 'cs' ? cs : en;
  const dialog = document.createElement('dialog');
  dialog.className = 'site-image-dialog';
  dialog.innerHTML = '<div class="site-image-toolbar"><strong></strong><button type="button" class="site-image-size"></button><a target="_blank" rel="noopener"></a><button type="button" class="site-image-close"></button></div><div class="site-image-viewport" tabindex="0"><img alt=""></div><p></p>';
  dialog.setAttribute('aria-label',t('Detail původního snímku','Original screenshot detail'));
  document.body.append(dialog);
  const picture=dialog.querySelector('img'), viewport=dialog.querySelector('.site-image-viewport'), size=dialog.querySelector('.site-image-size'), original=dialog.querySelector('a'), close=dialog.querySelector('.site-image-close');
  let opener, native=false;
  function labels() {
    size.textContent=native?t('Přizpůsobit oknu','Fit to window'):t('Původní velikost 1:1','Original size 1:1');
    size.setAttribute('aria-pressed',String(native));
    original.textContent=t('Otevřít originál ↗','Open original ↗');
    close.textContent=t('Zavřít · Esc','Close · Esc');
    dialog.querySelector('p').textContent=t('Režim 1:1 používá jeden pixel zdroje na jeden CSS pixel. Větší obrázek posouvej; na displejích s vysokým DPI může prohlížeč použít více fyzických pixelů.','1:1 uses one source pixel per CSS pixel. Scroll larger images; high-DPI displays may use multiple physical pixels.');
  }
  size.addEventListener('click',()=>{native=!native;dialog.classList.toggle('is-native',native);labels();});
  close.addEventListener('click',()=>dialog.close());
  dialog.addEventListener('click',e=>{if(e.target===dialog)dialog.close();});
  dialog.addEventListener('close',()=>{picture.removeAttribute('src');opener?.focus();});
  window.addEventListener('framelab:language',labels);
  function open(image,trigger) {
    opener=trigger;native=false;dialog.classList.remove('is-native');
    picture.onload=()=>{picture.style.setProperty('--source-width',picture.naturalWidth+'px');dialog.querySelector('strong').textContent=`${picture.naturalWidth} × ${picture.naturalHeight} px`;};
    picture.src=trigger.tagName==='A'?trigger.href:(image.currentSrc||image.src);picture.alt=image.alt;
    picture.style.setProperty('--source-width',image.naturalWidth+'px');
    original.href=picture.src;
    dialog.querySelector('strong').textContent=`${image.naturalWidth} × ${image.naturalHeight} px`;
    labels();dialog.showModal();viewport.scrollTo(0,0);close.focus();
  }
  function register(image) {
    if(image.dataset.imageReady || image.closest('.site-image-dialog,.guide-lightbox,.wipe,.compare-stage,.enhancer-stage') || !image.naturalWidth || image.naturalWidth<360)return;
    // Interactive before/after images and SVG callouts have their own controls.
    if(image.closest('[data-no-image-viewer]') || image.id?.startsWith('enhancer-') || image.classList.contains('before') || image.classList.contains('after'))return;
    const anchor=image.closest('a');
    if(anchor && !/\.(png|jpe?g|webp)(?:[?#]|$)/i.test(anchor.href))return;
    image.dataset.imageReady='true';
    image.style.maxWidth=`min(100%, ${image.naturalWidth}px)`;
    const trigger=anchor||image;
    if(!anchor){image.tabIndex=0;image.setAttribute('role','button');}
    trigger.classList.add('site-zoomable');
    trigger.addEventListener('click',e=>{if(e.ctrlKey||e.metaKey||e.shiftKey||e.altKey)return;e.preventDefault();open(image,trigger);});
    if(!anchor)trigger.addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();open(image,trigger);}});
    trigger.title=t('Zvětšit původní snímek','Enlarge original screenshot');
  }
  document.querySelectorAll('main img').forEach(image=>{if(image.complete)register(image);image.addEventListener('load',()=>register(image));});
})();
