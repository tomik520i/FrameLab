(() => {
  'use strict';
  const $ = s => document.querySelector(s);
  let language = 'en', manual = false, translations = [], reviews = new Map(), chart = [], page = 0;
  const pageSize = 25;
  const labels = {en:{all:'All',good:'No clear error',minor:'Minor issue',major:'Material error',unjudgeable:'Insufficient context',not_reviewed:'Not reviewed'},cs:{all:'Vše',good:'Bez zjevné chyby',minor:'Menší nepřesnost',major:'Podstatná chyba',unjudgeable:'Nedostatek kontextu',not_reviewed:'Nehodnoceno'}};
  const t = (en,cs) => language === 'cs' ? cs : en;
  function el(tag,text,cls){const n=document.createElement(tag);if(text!==undefined)n.textContent=text;if(cls)n.className=cls;return n}
  function applyLanguage(lang,save=false){language=lang==='cs'?'cs':'en';document.documentElement.lang=language;
    document.querySelectorAll('[data-en][data-cs]').forEach(e=>e.textContent=e.dataset[language]);
    $('#language').textContent=language==='en'?'CZ':'EN';$('#language').setAttribute('aria-label',t('Přepnout do češtiny','Switch to English'));
    if(save){manual=true;try{localStorage.setItem('framelab-language',language)}catch{}}
    render();draw();
  }
  $('#language').addEventListener('click',()=>applyLanguage(language==='en'?'cs':'en',true));
  const requested=new URLSearchParams(location.search).get('lang');let stored;try{stored=localStorage.getItem('framelab-language')}catch{}
  if(['en','cs'].includes(requested))applyLanguage(requested,true);else if(['en','cs'].includes(stored)){manual=true;applyLanguage(stored)}else fetch('../../locale.php',{cache:'no-store'}).then(r=>r.ok?r.json():Promise.reject()).then(r=>{if(!manual)applyLanguage(r.language)}).catch(()=>{});
  const time = seconds => {const v=Math.floor(seconds);return [Math.floor(v/3600),Math.floor(v/60)%60,v%60].map(x=>String(x).padStart(2,'0')).join(':')};
  function render(){
    if(!translations.length)return;
    const query=$('#search').value.toLocaleLowerCase().trim(), grade=$('#grade').value;
    const selected=translations.filter(r=>{const review=reviews.get(r.id),g=review?.grade||'not_reviewed';
      return (grade==='all'||grade===g)&&(!query||[String(r.id),'#'+r.id,String(r.job_id),r.source,r.translation,r.ocr_source,review?.note_en,review?.note_cs].filter(Boolean).join('\n').toLocaleLowerCase().includes(query));});
    const pages=Math.max(1,Math.ceil(selected.length/pageSize));page=Math.max(0,Math.min(page,pages-1));
    $('#result-count').textContent=t(`${selected.length} of ${translations.length} jobs · all 1,584 unique pairs reviewed · 0 unreviewed`,`${selected.length} z ${translations.length} úloh · všech 1 584 unikátních dvojic posouzeno · nehodnoceno 0`);
    $('#page-number').textContent=`${page+1} / ${pages}`;$('#previous').disabled=page===0;$('#next').disabled=page>=pages-1;
    const list=$('#translation-list');list.replaceChildren();
    for(const r of selected.slice(page*pageSize,(page+1)*pageSize)){
      const rev=reviews.get(r.id),grade=rev?.grade||'not_reviewed',card=el('article',undefined,'translation');card.id='translation-'+r.id;
      const meta=el('div',undefined,'meta');meta.append(el('span',`#${r.id} · ${time(r.time_s)} · OCR job ${r.job_id}`),el('span',labels[language][grade],'badge '+grade));card.append(meta);
      const pair=el('div',undefined,'pair');for(const [lang,label,text] of [['en','EN / LOGGED SOURCE',r.source],['cs','CS / OUTPUT',r.translation]]){const col=el('div');col.lang=lang;col.append(el('small',label),el('div',text));pair.append(col)}card.append(pair);
      if(rev){const note=el('p',rev['note_'+language]||'Review note (Czech): '+rev.note_cs);note.lang=rev['note_'+language]?language:'cs';card.append(note);}
      const timing=`${t('Translation','Překlad')}: ${r.translation_ms.toFixed(1)} ms${r.cache_hit?' / CACHE':''} · ${t('Caption delay','Zpoždění titulku')}: ${r.displayed?(r.caption_delay_ms/1000).toFixed(3)+' s':t('NOT DISPLAYED','NEZOBRAZENO')}`;card.append(el('p',timing));
      const details=el('details');details.append(el('summary',t('Original OCR fragment and timing details','Původní OCR fragment a podrobnosti časů')));
      const display={utc:r.utc,time_s:r.time_s,ocr_source:r.ocr_source,ocr_ms:r.ocr_ms,ocr_queue_ms:r.ocr_queue_ms,confidence:r.confidence,translation_queue_ms:r.translation_queue_ms,display_time_s:r.display_time_s,displayed:r.displayed,cache_hit:r.cache_hit,reviewed_pair_id:rev?.reviewed_pair_id,exact_duplicate_reuse:rev?.exact_duplicate_reuse,judgment_origin:rev?.judgment_origin};details.append(el('pre',JSON.stringify(display,null,2)));card.append(details);list.append(card);
    }
  }
  $('#search').addEventListener('input',()=>{page=0;render()});$('#grade').addEventListener('change',()=>{page=0;render()});
  for(const [id,delta] of [['previous',-1],['next',1]])$('#'+id).addEventListener('click',()=>{page+=delta;render();$('#result-count').scrollIntoView({block:'start'})});
  async function get(path){const r=await fetch(path);if(!r.ok)throw Error(path+': '+r.status);return r.json()}
  Promise.all([get('data/translations.json'),get('data/quality-full-review.json')]).then(([rows,review])=>{translations=rows;reviews=new Map(review.rows.map(r=>[r.id,r]));if(reviews.size!==rows.length)throw Error('Incomplete review');render()}).catch(()=>{$('#result-count').textContent=t('The interactive list failed to load. Download the complete JSON/CSV below.','Interaktivní seznam se nepodařilo načíst. Úplný JSON/CSV stáhneš níže.')});
  function draw(){if(!chart.length)return;const canvas=$('#performance-chart'),ctx=canvas.getContext('2d'),ratio=devicePixelRatio||1,w=canvas.clientWidth,h=canvas.clientHeight;canvas.width=w*ratio;canvas.height=h*ratio;ctx.setTransform(ratio,0,0,ratio,0,0);ctx.clearRect(0,0,w,h);
    const left=36,right=w-12,top=12,bottom=h-30,total=chart.at(-1).end_s,x=v=>left+v/total*(right-left),y=v=>bottom-Math.min(110,Math.max(0,v))/110*(bottom-top);
    ctx.font='12px monospace';ctx.fillStyle='#afbba6';ctx.strokeStyle='#39452f';ctx.lineWidth=1;
    for(let v=0;v<=100;v+=25){ctx.beginPath();ctx.moveTo(left,y(v));ctx.lineTo(right,y(v));ctx.stroke();ctx.fillText(String(v),2,y(v)+4)}
    for(let m=0;m<=150;m+=30)ctx.fillText(m+'m',x(m*60)-8,h-7);
    for(const [key,color] of [['output_fps','#bdff57'],['gpu_percent','#70c7b0']]){ctx.strokeStyle=color;ctx.lineWidth=1.5;ctx.beginPath();let started=false;for(const r of chart){if(!Number.isFinite(r[key])){started=false;continue}if(started)ctx.lineTo(x(r.time_s),y(r[key]));else{ctx.moveTo(x(r.time_s),y(r[key]));started=true}}ctx.stroke()}
    const selected=Math.min(chart.length-1,Number($('#time').value));const r=chart[selected];ctx.strokeStyle='#eef1e6';ctx.beginPath();ctx.moveTo(x(r.time_s),top);ctx.lineTo(x(r.time_s),bottom);ctx.stroke();
    $('#chart-readout').textContent=`${time(r.time_s)}–${time(r.end_s)} | OUTPUT ${r.output_fps.toFixed(1)} fps | GPU ${r.gpu_percent?.toFixed(1)??'N/A'}% | ${r.gpu_power_w?.toFixed(1)??'N/A'} W | ${t('gaps > 50 ms','mezery > 50 ms')}: ${r.gaps_over_50ms}`;
  }
  get('data/chart.json').then(rows=>{chart=rows;$('#time').max=rows.length-1;draw();new ResizeObserver(draw).observe($('#performance-chart'))}).catch(()=>{$('#chart-readout').textContent=t('Chart unavailable; download chart.json or metrics.csv.','Graf není dostupný; stáhni chart.json nebo metrics.csv.')});
  $('#time').addEventListener('input',draw);
  $('#performance-chart').addEventListener('pointermove',e=>{if(!chart.length)return;const r=e.currentTarget.getBoundingClientRect(),fraction=Math.max(0,Math.min(1,(e.clientX-r.left-36)/(r.width-48)));$('#time').value=Math.round(fraction*(chart.length-1));draw()});
  get('data/download-sizes.json').then(sizes=>document.querySelectorAll('.downloads a').forEach(a=>{const name=a.getAttribute('href').split('/').at(-1),r=sizes[name];if(r)a.append(el('small',r.bytes>=1048576?(r.bytes/1048576).toFixed(2)+' MiB':(r.bytes/1024).toFixed(1)+' KiB'))})).catch(()=>{});
})();
