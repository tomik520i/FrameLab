'use strict';
(() => {
  const assetRoot=new URL('./',document.currentScript.src),asset=p=>new URL(p,assetRoot).href;
  const root = document.querySelector('#enhancer');
  if (!root) return;
  const stage = root.querySelector('.nr-stage');
  const before = root.querySelector('#nr-before');
  const after = root.querySelector('#nr-after');
  const divider = root.querySelector('#nr-divider');
  const strength = root.querySelector('#nr-strength');
  const frame = root.querySelector('#nr-frame');
  const status = root.querySelector('#nr-status');
  let sample = 'combat', generation = 0;
  function localize() {
    const cs = document.documentElement.lang.startsWith('cs');
    root.querySelectorAll('[data-nr-en]').forEach(el => {el.textContent = el.dataset[cs ? 'nrCs' : 'nrEn'];});
  }
  new MutationObserver(localize).observe(document.documentElement,{attributes:true,attributeFilter:['lang']});
  localize();
  function controls() {
    stage.style.setProperty('--nr-split', divider.value + '%');
    stage.style.setProperty('--nr-strength', Math.round(Number(strength.value)*255/100)/255);
    root.querySelector('#nr-divider-value').value = divider.value + ' %';
    root.querySelector('#nr-strength-value').value = strength.value + ' %';
  }
  divider.addEventListener('input', controls);strength.addEventListener('input', controls);
  function move(event) {
    const box = stage.getBoundingClientRect();
    divider.value = Math.round(Math.max(0,Math.min(1,(event.clientX-box.left)/box.width))*100);controls();
  }
  stage.addEventListener('pointerdown', event => {stage.setPointerCapture(event.pointerId);move(event);});
  stage.addEventListener('pointermove', event => {if(stage.hasPointerCapture(event.pointerId))move(event);});
  stage.addEventListener('pointerup', event => {if(stage.hasPointerCapture(event.pointerId))stage.releasePointerCapture(event.pointerId);});
  async function loadPair() {
    const token = ++generation, key = sample, index = frame.value;
    status.textContent = document.documentElement.lang.startsWith('cs') ? 'Načítání stejného páru…' : 'Loading matching frame pair…';
    try {
      const paths = ['original','nr100'].map(mode => asset(`enhancer/snimky/${key}-${index}-${mode}.png`));
      const images = await Promise.all(paths.map(src => new Promise((resolve,reject) => {const img=new Image();img.onload=()=>resolve(img);img.onerror=reject;img.src=src;})));
      if(token!==generation)return;
      if(images[0].naturalWidth!==images[1].naturalWidth || images[0].naturalHeight!==images[1].naturalHeight)throw new Error('Size mismatch');
      before.src=paths[0];after.src=paths[1];
      root.querySelector('#nr-original-png').href=paths[0];root.querySelector('#nr-enhanced-png').href=paths[1];
      root.querySelectorAll('[data-nr-sample]').forEach(button=>button.setAttribute('aria-pressed',String(button.dataset.nrSample===key)));
      status.textContent='';
    } catch (error) {
      if(token===generation)status.textContent=document.documentElement.lang.startsWith('cs')?'Snímky se nepodařilo načíst. Zkus jiný snímek nebo otevři celou galerii.':'Images failed to load. Try another frame or open the full gallery.';
    }
  }
  root.querySelectorAll('[data-nr-sample]').forEach(button=>button.addEventListener('click',()=>{sample=button.dataset.nrSample;loadPair();}));
  frame.addEventListener('change',loadPair);controls();loadPair();
})();
