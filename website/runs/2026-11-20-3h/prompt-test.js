(() => {
 const $=s=>document.querySelector(s);let rows=[],page=0,lang=new URLSearchParams(location.search).get('lang')==='cs'?'cs':'en';
 const node=(tag,text)=>{const e=document.createElement(tag);e.textContent=text;return e};
 function language(){document.documentElement.lang=lang;document.querySelectorAll('[data-en]').forEach(e=>e.textContent=e.dataset[lang]);}
 $('#language').onclick=()=>{lang=lang==='en'?'cs':'en';language()};language();
 function render(){const q=$('#search').value.trim().toLowerCase(),phase=$('#phase').value,profile=$('#profile').value,change=$('#change').value;
 const selected=rows.filter(r=>(phase==='all'||phase===r.phase)&&(profile==='all'||profile===r.profile)&&(!q||['#'+r.id,r.source,r.baseline,r.translation,r.note_cs,r.baseline_note_cs].join('\n').toLowerCase().includes(q))&&(change==='all'||change==='changed'&&r.baseline!==r.translation||change==='fixed'&&r.baseline_grade==='major'&&['good','minor'].includes(r.grade)||change==='regression'&&['good','minor'].includes(r.baseline_grade)&&r.grade==='major'));
 const pages=Math.max(1,Math.ceil(selected.length/20));page=Math.max(0,Math.min(page,pages-1));$('#count').textContent=selected.length+' / '+rows.length+' comparisons · 0 unreviewed';$('#page').textContent=(page+1)+' / '+pages;$('#prev').disabled=page===0;$('#next').disabled=page===pages-1;$('#list').replaceChildren();
 for(const r of selected.slice(page*20,(page+1)*20)){const a=node('article','');a.className='translation';a.append(node('small',`#${r.id} · ${r.phase} · ${r.profile}`),node('h3',r.source));const pair=node('div','');pair.className='pair';for(const [title,text,grade,ms,note] of [['Baseline',r.baseline,r.baseline_grade,r.baseline_ms,r.baseline_note_cs],[r.profile,r.translation,r.grade,r.wall_ms,r.note_cs]]){const c=node('div','');c.append(node('small',`${title} · ${grade} · ${ms.toFixed(1)} ms`),node('p',text),node('p',note));pair.append(c)}a.append(pair);$('#list').append(a)}
 }
 for(const id of ['search','phase','profile','change'])$('#'+id).addEventListener(id==='search'?'input':'change',()=>{page=0;render()});$('#prev').onclick=()=>{page--;render()};$('#next').onclick=()=>{page++;render()};
 fetch('data/prompt-comparison.json').then(r=>{if(!r.ok)throw Error(r.status);return r.json()}).then(r=>{rows=r;render()}).catch(()=>$('#count').textContent='Unable to load; download JSON below / Stáhni JSON níže.');
})();
