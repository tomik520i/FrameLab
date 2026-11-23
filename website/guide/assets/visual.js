(() => {
  const dialog = document.querySelector('.guide-lightbox');
  if (!dialog) return;
  const body=dialog.querySelector('.lightbox-body'),close=dialog.querySelector('.lightbox-close');
  const en=document.documentElement.lang==='en';let opener;
  document.querySelectorAll('.shot-button').forEach(button=>button.addEventListener('click',()=>{
    opener=button;body.replaceChildren();
    const figure=button.closest('figure'),svg=button.querySelector('svg').cloneNode(true);
    const width=svg.viewBox.baseVal.width;
    svg.querySelectorAll('[id]').forEach(el=>{
      const old=el.id,next=old+'-zoom';
      svg.querySelectorAll('[marker-end]').forEach(line=>{if(line.getAttribute('marker-end')===`url(#${old})`)line.setAttribute('marker-end',`url(#${next})`);});el.id=next;
    });
    const tools=document.createElement('div');tools.className='guide-image-tools';
    const toggle=document.createElement('button');toggle.type='button';toggle.textContent=en?'Original size 1:1':'Původní velikost 1:1';toggle.setAttribute('aria-pressed','false');
    const original=document.createElement('a');original.href=svg.querySelector('image').getAttribute('href');original.target='_blank';original.rel='noopener';original.textContent=en?'Full original ↗':'Celý originál ↗';
    tools.append(toggle,original);
    const viewport=document.createElement('div');viewport.className='guide-image-viewport';viewport.tabIndex=0;viewport.style.setProperty('--shot-width',width+'px');viewport.append(svg);
    toggle.onclick=()=>{const native=viewport.classList.toggle('is-native');toggle.setAttribute('aria-pressed',String(native));toggle.textContent=native?(en?'Fit to window':'Přizpůsobit oknu'):(en?'Original size 1:1':'Původní velikost 1:1');};
    body.append(tools,viewport);
    for(const name of ['.callout-legend','figcaption']){const node=figure.querySelector(name);if(node)body.append(node.cloneNode(true));}
    dialog.showModal();close.focus();
  }));
  close.addEventListener('click',()=>dialog.close());
  dialog.addEventListener('click',e=>{if(e.target===dialog)dialog.close();});
  dialog.addEventListener('close',()=>{body.replaceChildren();opener?.focus();});
})();
