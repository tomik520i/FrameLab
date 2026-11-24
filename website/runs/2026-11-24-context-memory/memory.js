/* Real recorded translations; no inference, telemetry or external API calls. */
(()=>{'use strict';
const t=(cs,en)=>window.FrameLabLocale?.language==='cs'?cs:en;
const names={baseline:['Bez paměti','No memory'],naive:['Syrová historie','Raw history'],guarded:['Omezená paměť','Bounded memory'],timeline:['Přepis s časy','Timestamped transcript']};
const el=(tag,text,cls)=>{const x=document.createElement(tag);if(text!==undefined)x.textContent=text;if(cls)x.className=cls;return x};
let data,model='gpt-6-luna',caseId='game-1399';
const modelSelect=document.querySelector('#memory-model'),caseSelect=document.querySelector('#memory-case');
function label(id){return id==='server-gemma4-26b-a4b-q5ks'?t('Gemma 4 26B A4B Q5_K_S · externí RTX 3090','Gemma 4 26B A4B Q5_K_S · external RTX 3090'):data.models[id]}
function scoreTable(){const wrap=document.querySelector('#memory-scores');wrap.replaceChildren();const scroll=el('div',undefined,'memory-table-wrap'),table=el('table'),head=el('thead'),tr=el('tr');
 [t('Model / režim','Model / mode'),t('Skóre /100','Score /100'),t('Odezva: medián / P95','Latency: median / P95'),t('Známý rod','Known gender'),t('Slovo / jméno','Word / name')].forEach(x=>tr.append(el('th',x)));head.append(tr);table.append(head);const body=el('tbody');
 for(const id of Object.keys(data.models)){const title=el('tr',undefined,'memory-model-row'),th=el('th',label(id));th.colSpan=5;title.append(th);body.append(title);for(const mode of data.modes){const s=data.summary.find(x=>x.model===id&&x.mode===mode),r=el('tr');[t(...names[mode]),s.score.toFixed(1),`${(s.latency_ms.median/1000).toFixed(2)} / ${(s.latency_ms.p95/1000).toFixed(2)} s`,`${s.gender_pass}/${s.gender_n}`,`${s.lexical_pass}/${s.lexical_n}`].forEach(x=>r.append(el('td',x)));body.append(r)}}table.append(body);scroll.append(table);wrap.append(scroll);
 const network=data.summary.filter(x=>x.model==='server-gemma4-26b-a4b-q5ks'&&x.server_compute_ms.n);
 if(network.length)wrap.append(el('p',t('Externí Gemma: medián samotného výpočtu hlášeného serverem podle režimu: ','External Gemma: median server-reported compute by mode: ')+network.map(x=>`${t(...names[x.mode])} ${(x.server_compute_ms.median/1000).toFixed(2)} s`).join(' · '),'fine'));
}
function example(){const c=data.cases.find(x=>x.id===caseId),wrap=document.querySelector('#memory-example');wrap.replaceChildren();wrap.append(el('p',c.domain==='game_raw_ocr'?t('Skutečné OCR z herní relace','Actual OCR from the gameplay session'):t('Vytvořená zátěžová scéna; profily postav jsou zadané předem','Authored stress scene; character profiles supplied in advance'),'eyebrow'));wrap.append(el('h3',c.source,'memory-source'));
 const grid=el('div',undefined,'memory-comparison');for(const mode of data.modes){const r=data.records.find(x=>x.model===model&&x.id===caseId&&x.mode===mode),card=el('article',undefined,'card');card.append(el('h4',t(...names[mode])));card.append(el('p',r.translation||t('(Prázdný výstup)','(Empty output)'),'memory-output'));let grade;
 if(r.review.score!==undefined&&r.review.score!==null)grade=`${r.review.score}/4`;
 else if(typeof r.review.feature_pass==='boolean')grade=r.review.feature_pass?t('Ověřený znak: ano','Checked feature: pass'):t('Ověřený znak: ne','Checked feature: fail');
 else grade=t('Bez číselného hodnocení','Not numerically scored');
 card.append(el('p',`${grade} · ${(r.ms/1000).toFixed(2)} s`,'memory-grade'));
 card.append(el('p',t(r.review.note_cs||r.review.note_en,r.review.note_en),'fine'));
 if(r.error)card.append(el('p',r.error,'notice warning'));
 const detail=el('details'),summary=el('summary',t('Co model dostal z paměti','What memory the model received'));detail.append(summary);
 const ctx=c.context[mode];if(!ctx)detail.append(el('p',t('Pouze aktuální titulek.','Only the current subtitle.')));else{detail.append(el('p',t('Aktuální mluvčí: ','Current speaker: ')+(ctx.current_speaker||t('neznámý','unknown'))));
 if(ctx.verified_profiles.length)detail.append(el('pre',JSON.stringify(ctx.verified_profiles,null,2),'memory-context'));
 const list=el('ol',undefined,'memory-history');for(const row of ctx.history){const item=el('li');item.append(el('span',`${row.age_s.toFixed(1)} s ${t('před cílem','before target')} · ${row.speaker||t('neznámý mluvčí','unknown speaker')}`,'memory-time'));item.append(el('p',row.source));if(row.segment!==undefined)item.append(el('small',t('Časový úsek ','Temporal segment ')+row.segment));list.append(item)}detail.append(list);if(!ctx.history.length)detail.append(el('p',t('Historie je prázdná nebo vypršela.','History is empty or expired.')))}card.append(detail);grid.append(card)}wrap.append(grid);
 const note=el('p',t('Čas je vůči dokončení cílového OCR. Označení úseku nepotvrzuje společný rozhovor. Hodnocení znaku (například rodu) není hodnocením celé věty.','Times are relative to target OCR completion. A segment label does not certify a shared conversation. A feature assessment (such as gender) is not an assessment of the entire sentence.'),'fine');wrap.append(note);
}
function render(){if(!data)return;modelSelect.replaceChildren();for(const id of Object.keys(data.models)){const o=el('option',label(id));o.value=id;o.selected=id===model;modelSelect.append(o)}caseSelect.replaceChildren();for(const row of data.cases){const o=el('option',row.id+' · '+row.source.slice(0,76));o.value=row.id;o.selected=row.id===caseId;caseSelect.append(o)}document.querySelector('#memory-prev').setAttribute('aria-label',t('Předchozí případ','Previous case'));document.querySelector('#memory-next').setAttribute('aria-label',t('Následující případ','Next case'));scoreTable();example()}
modelSelect.addEventListener('change',()=>{model=modelSelect.value;example()});caseSelect.addEventListener('change',()=>{caseId=caseSelect.value;example()});
for(const [id,delta] of [['memory-prev',-1],['memory-next',1]])document.getElementById(id).onclick=()=>{if(!data)return;const i=data.cases.findIndex(x=>x.id===caseId);caseId=data.cases[(i+delta+data.cases.length)%data.cases.length].id;caseSelect.value=caseId;example()};
addEventListener('framelab:language',render);
fetch('data/report.json').then(r=>{if(!r.ok)throw Error();return r.json()}).then(x=>{data=x;render()}).catch(()=>{document.getElementById('memory-example').textContent=t('Výsledky se nepodařilo načíst. Obnov stránku nebo stáhni CSV níže.','Could not load results. Refresh the page or download the CSV below.')});
})();
