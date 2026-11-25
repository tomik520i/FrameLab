/* Shared, keyboard-accessible navigation. No tracking or external requests. */
(()=>{'use strict';const root=new URL('./',document.currentScript.src),url=p=>new URL(p,root).href;
const text=(cs,en)=>window.FrameLabLocale?.language==='cs'?cs:en;
const groups=[
 ['Začít','Get started',[
  ['Stáhnout FrameLab','Download FrameLab','#download','ZIP, instalace, installation'],
  ['Co aplikace umí','What the app does','#features','capture fullscreen subtitles'],
  ['Obrazový návod','Illustrated walkthrough','guide/visual-en.html','setup první spuštění']]],
 ['Ukázky a porovnání','Demos & comparisons',[
  ['Obraz před a po','Before & after','showcase/#enhancer','enhancer NR kvalita image'],
  ['Generované snímky','Generated frames','showcase/#evidence','RIFE VFG optical flow FPS video'],
  ['Překlady mezi jazyky','Translations across languages','runs/2026-11-24-language-lab/','Japanese Czech čeština japonština Gemma'],
  ['Paměť rozhovoru','Conversation memory','runs/2026-11-24-context-memory/','history context kontext časy CUNI Luna Gemma server'],
  ['75 scén v aplikaci: paměť a chyby','75 in-app scenes: memory & failures','runs/2026-11-24-context75/','context OCR filler latency Japanese čeština japonština 300'],
  ['500 překladů EN → CS','500 EN → CS translations','runs/2026-11-23-direct-review/','CUNI OpenAI OPUS skóre quality']]],
 ['Výsledky měření','Test results',[
  ['Přehled výsledků','Results overview','results/','FPS odezva latency score měření'],
  ['Metody a co čísla znamenají','Methods & reading the numbers','results/#methods','methodology metodika scoring'],
  ['Testované počítače','Tested hardware','results/#hardware','RTX 4050 5070 Intel AMD'],
  ['Podrobné logy a data','Detailed logs & data','results/#reports','CSV JSON audit raw']]],
 ['Návody a pomoc','Guides & help',[
  ['Všechny návody','All guides','help/','help podpora instalace'],
  ['Titulky a modely','Subtitles & models','guide/subtitles-en.html','GPU překlad translation'],
  ['Vlastní AI server (vývoj)','Custom AI server (development)','guide/custom-ai-en.html','LAN custom API llama.cpp Ollama memory'],
  ['Remote Play a okno hry','Remote Play & game windows','guide/remote-subtitles-en.html','PS Xbox API overlay'],
  ['Čistý fullscreen','Clean fullscreen','guide/fullscreen-en.html','F11 display monitor'],
  ['Licence a volitelné soubory','Licenses & optional files','licenses/','NVIDIA DLL license']]]
];
const header=document.createElement('header');header.className='site-header';header.innerHTML='<a class="site-brand" href="'+url('')+'">FRAME<span>/</span>LAB</a><nav class="site-primary" aria-label="FrameLab"></nav><div class="site-actions"><button type="button" class="site-menu-toggle" aria-haspopup="dialog" aria-expanded="false"><svg viewBox="0 0 24 24" width="21" height="21" aria-hidden="true"><path d="M3 6h18M3 12h18M3 18h18" fill="none" stroke="currentColor" stroke-width="1.7"/></svg><span>Menu</span></button></div>';
const primary=header.querySelector('.site-primary');[['Začít','Start',''],['Porovnání','Compare','showcase/'],['Výsledky','Results','results/'],['Návody','Guides','help/']].forEach(([cs,en,path])=>{const a=document.createElement('a');a.href=url(path);a.dataset.cs=cs;a.dataset.en=en;a.textContent=text(cs,en);primary.append(a)});
const language=document.querySelector('[data-site-language]');if(language)header.querySelector('.site-actions').prepend(language);
document.querySelector('.site-language-bar')?.remove();document.body.prepend(header);
document.querySelectorAll('header.wrap.nav').forEach(h=>h.hidden=true);
const main=document.querySelector('main');if(main&&!main.id)main.id='main';
const skip=document.createElement('a');skip.className='site-skip';skip.href='#'+(main?.id||'main');skip.dataset.cs='Přejít na obsah';skip.dataset.en='Skip to content';skip.textContent=text(skip.dataset.cs,skip.dataset.en);document.body.prepend(skip);
const dialog=document.createElement('dialog');dialog.className='site-menu';dialog.setAttribute('aria-labelledby','site-menu-title');dialog.innerHTML='<div class="site-menu-heading"><h2 id="site-menu-title"></h2><button type="button" class="site-menu-close"></button></div><label class="site-search-label" for="site-search"></label><input id="site-search" type="search" autocomplete="off"><div class="site-menu-grid"></div><p class="site-menu-empty" hidden></p>';
const grid=dialog.querySelector('.site-menu-grid');for(const [cs,en,links] of groups){const section=document.createElement('section');const h=document.createElement('h3');h.dataset.cs=cs;h.dataset.en=en;h.textContent=text(cs,en);section.append(h);for(const [a,b,path,keywords] of links){const link=document.createElement('a');link.href=url(path);link.dataset.cs=a;link.dataset.en=b;link.dataset.search=[a,b,keywords].join(' ').toLocaleLowerCase();link.textContent=text(a,b);link.className='site-menu-link';section.append(link)}grid.append(section)}document.body.append(dialog);
const button=header.querySelector('.site-menu-toggle'),search=dialog.querySelector('input');
function filter(){const q=search.value.toLocaleLowerCase().normalize('NFD').replace(/[\u0300-\u036f]/g,'');let n=0;for(const a of grid.querySelectorAll('a')){a.hidden=!a.dataset.search.normalize('NFD').replace(/[\u0300-\u036f]/g,'').includes(q);if(!a.hidden)n++}for(const s of grid.children)s.hidden=![...s.querySelectorAll('a')].some(a=>!a.hidden);dialog.querySelector('.site-menu-empty').hidden=n>0}
function labels(){dialog.querySelector('h2').textContent=text('Co chceš zjistit?','What would you like to explore?');dialog.querySelector('.site-menu-close').textContent=text('Zavřít ×','Close ×');dialog.querySelector('label').textContent=text('Najít návod, funkci nebo měření','Find a guide, feature or measurement');search.placeholder=text('Například: japonština, FPS, fullscreen…','For example: Japanese, FPS, fullscreen…');dialog.querySelector('.site-menu-empty').textContent=text('Nic nenalezeno. Zkus kratší výraz.','No matches. Try a shorter search.');button.setAttribute('aria-label',text('Otevřít menu webu','Open website menu'));header.querySelectorAll('[data-cs]').forEach(e=>e.textContent=text(e.dataset.cs,e.dataset.en))}
button.addEventListener('click',()=>{dialog.showModal();button.setAttribute('aria-expanded','true');search.focus()});dialog.querySelector('.site-menu-close').onclick=()=>dialog.close();dialog.addEventListener('click',e=>{if(e.target===dialog||e.target.closest('a'))dialog.close()});dialog.addEventListener('close',()=>{button.setAttribute('aria-expanded','false');button.focus()});dialog.addEventListener('keydown',e=>{if(e.key==='Escape'){e.preventDefault();e.stopPropagation();dialog.close();}});search.addEventListener('input',filter);addEventListener('framelab:language',labels);labels();
})();
