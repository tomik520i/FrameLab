(() => {'use strict';
 const root=new URL('./',document.currentScript.src),$=s=>document.querySelector(s),locale=()=>window.FrameLabLocale?.language||document.documentElement.lang,t=(cs,en)=>locale()==='cs'?cs:en;
 const names={cs:['čeština','Czech'],en:['angličtina','English'],ja:['japonština','Japanese'],de:['němčina','German'],es:['španělština','Spanish'],fr:['francouzština','French']};
 const name=x=>names[x][locale()==='cs'?0:1],pair=(a,b)=>`${name(a)} → ${name(b)}`,model=m=>m.startsWith('cuni')?'CUNI-MH 7B Q3_K_M':'TranslateGemma 4B Q4_K_M',num=(n,d=0)=>new Intl.NumberFormat(locale(),{maximumFractionDigits:d,minimumFractionDigits:d}).format(n);
 const pairFilter=$('#pair-filter'),modelFilter=$('#model-filter'),scoreFilter=$('#score-filter'),sortFilter=$('#sort-filter');let report,page=0;const pageSize=12;
 function element(tag,text,cls){const e=document.createElement(tag);e.textContent=text;if(cls)e.className=cls;return e;}
 function summary(){const table=$('#language-scores');table.replaceChildren();for(const g of report.groups){const tr=document.createElement('tr');tr.append(element('td',model(g.model)));const td=document.createElement('td'),b=element('button',pair(g.source,g.target),'table-pair');b.type='button';b.onclick=()=>{pairFilter.value=g.source+'-'+g.target;modelFilter.value=g.model;scoreFilter.value='all';page=0;render();$('#examples').scrollIntoView({behavior:'auto'});};td.append(b);tr.append(td);for(const [n,d,cls,suffix] of [[g.n,0,'',''],[g.score,1,'score',''],[g.meaning,2,'',''],[g.fluency,2,'',''],[g.median_ms,0,'',' ms'],[g.p95_ms,0,'',' ms'],[g.major_meaning_errors,0,g.major_meaning_errors?'bad':'',' / 12']])tr.append(element('td',num(n,d)+suffix,cls));table.append(tr);}}
 function render(){
  let rows=report.rows.filter(r=>(pairFilter.value==='all'||r.source_language+'-'+r.target_language===pairFilter.value)&&(modelFilter.value==='all'||r.model===modelFilter.value)&&(scoreFilter.value==='all'||(scoreFilter.value==='errors'?r.review.meaning<=2:r.review.score===100)));
  if(sortFilter.value==='worst')rows.sort((a,b)=>a.review.score-b.review.score);if(sortFilter.value==='slowest')rows.sort((a,b)=>b.ms-a.ms);
  const pages=Math.max(1,Math.ceil(rows.length/pageSize));page=Math.min(page,pages-1);
  $('#case-count').textContent=t(`Vybráno ${rows.length} z 132 skutečných výstupů.`,`Showing ${rows.length} of 132 actual outputs.`);
  $('#page-count').textContent=t(`Strana ${page+1} / ${pages}`,`Page ${page+1} / ${pages}`);$('#previous').disabled=page===0;$('#next').disabled=page>=pages-1;
  const list=$('#language-cases');list.replaceChildren();
  for(const r of rows.slice(page*pageSize,(page+1)*pageSize)){
   const article=document.createElement('article');article.className='case';article.dataset.outputId=r.id;
   const head=element('div','','case-header');head.append(element('h3',`${pair(r.source_language,r.target_language)} · ${r.sentence_id.toUpperCase()}`),element('span',`${num(r.review.score,1)}/100 · ${num(r.ms)} ms`,r.review.meaning<=2?'bad':'score'));
   article.append(head,element('p',model(r.model),'fine'));const texts=element('div','','pair');
   for(const [label,value,lang] of [[t('Vstup','Input'),r.source,r.source_language],[t('Skutečný výstup','Actual output'),r.translation,r.target_language]]){const box=document.createElement('div'),quote=element('blockquote',value);quote.lang=lang;box.append(element('strong',label),quote);texts.append(box);}article.append(texts);
   const review=element('div','','review');review.append(element('strong',t(`Význam ${r.review.meaning}/4 · jazyk ${r.review.fluency}/4`,`Meaning ${r.review.meaning}/4 · fluency ${r.review.fluency}/4`)),element('p',r.review['note_'+locale()]));article.append(review);
   const ref=element('details','','reference'),quote=element('blockquote',r.reference);quote.lang=r.target_language;ref.append(element('summary',t('Jedna možná referenční formulace','One possible reference wording')),quote);article.append(ref);list.append(article);
  }
  if(!rows.length)list.append(element('p',t('Této kombinaci neodpovídá žádný výstup. CUNI bylo měřeno pouze pro EN → CS.','No outputs match these filters. CUNI was measured only for EN → CS.')));
 }
 function labels(){if(!report)return;const selected=pairFilter.value||'cs-ja';pairFilter.replaceChildren();const all=element('option',t('Všechny směry','All directions'));all.value='all';pairFilter.append(all);for(const key of new Set(report.groups.map(g=>g.source+'-'+g.target))){const [s,d]=key.split('-'),option=element('option',pair(s,d));option.value=key;pairFilter.append(option);}pairFilter.value=selected;summary();render();}
 [pairFilter,modelFilter,scoreFilter,sortFilter].forEach(e=>e.addEventListener('change',()=>{page=0;render();}));
 $('#previous').onclick=()=>{page--;render();$('#examples').scrollIntoView();};$('#next').onclick=()=>{page++;render();$('#examples').scrollIntoView();};window.addEventListener('framelab:language',labels);
 fetch(new URL('data/report.json',root)).then(r=>{if(!r.ok)throw Error(r.status);return r.json();}).then(r=>{report=r;labels();document.documentElement.dataset.labReady='true';}).catch(()=>{$('#case-count').textContent=t('Data se nenačetla. Obnov stránku nebo stáhni CSV níže.','Data could not load. Reload or download the CSV below.');document.documentElement.dataset.labReady='error';});
})();
