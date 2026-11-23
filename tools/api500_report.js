'use strict';
let report, page = 0;
const byId = id => document.getElementById(id);
const escape = text => String(text ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
function render() {
  if (!report) return;
  const model = byId('model').value;
  const query = byId('search').value.toLocaleLowerCase();
  const selected = c => Object.entries(c.models).filter(([id]) => !model || id === model);
  const cases = report.cases.filter(c => {
    const rows = selected(c);
    return (!query || [c.id, c.en, c.reference_cs, ...rows.map(([,r])=>r.translation)].join(' ').toLocaleLowerCase().includes(query)) &&
      (!byId('critical').checked || rows.some(([,r])=>r.rating?.critical)) &&
      (!byId('errors').checked || rows.some(([,r])=>r.error));
  });
  page = Math.max(0, Math.min(page, Math.ceil(cases.length / 20) - 1));
  const visible = cases.slice(page * 20, page * 20 + 20);
  byId('count').textContent = `${cases.length} z 500 replik · strana ${page + 1} / ${Math.max(1,Math.ceil(cases.length / 20))}`;
  byId('prev').disabled = page === 0;
  byId('next').disabled = (page + 1) * 20 >= cases.length;
  byId('cases').innerHTML = visible.map(c => `<article><h3>${escape(c.id)}</h3><blockquote lang="en">${escape(c.en)}</blockquote><p><b>Lidská reference:</b> ${escape(c.reference_cs)}</p><div class="candidates">${selected(c).map(([id,r])=>{
    const name=report.models.find(m=>m.model===id).label, score=r.rating;
    return `<section class="candidate"><strong>${escape(name)}</strong><p>${r.error?`<span class="bad">${escape(r.error)}</span>`:escape(r.translation)}</p><p class="score">${(r.ms/1000).toFixed(2)} s · chrF++ ${r.chrf_pp.toFixed(1)}${score?` · význam ${score.meaning}/4 · čeština ${score.fluency}/4`:''}</p>${score?`<p class="meta ${score.critical?'bad':''}">AI posouzení: ${escape(score.note_cs)}</p>`:''}</section>`;
  }).join('')}</div></article>`).join('');
}
for (const id of ['search','model','critical','errors']) byId(id).addEventListener('input',()=>{page=0;render();});
byId('prev').addEventListener('click',()=>{--page;render();});
byId('next').addEventListener('click',()=>{++page;render();});
async function getJson(path){const r=await fetch(path);if(!r.ok)throw Error('HTTP '+r.status);return r.json();}
getJson('data/viewer-index.json').then(async data=>{
  const chunks=[];
  // Bounded parallel downloads keep both the server and a phone responsive.
  for(let i=0;i<data.chunks.length;i+=4)chunks.push(...await Promise.all(data.chunks.slice(i,i+4).map(name=>getJson('data/'+name))));
  report={models:data.models,cases:chunks.flat()};render();
}).catch(()=>{byId('count').textContent='Data se nepodařilo načíst. Obnov stránku nebo stáhni CSV výše.';});
