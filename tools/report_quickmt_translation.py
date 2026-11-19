"""Build an auditable local report after all seven variants are reviewed."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import html
import json
from pathlib import Path
import statistics

from evaluate_quickmt_translation import PROFILES

NAMES = {'baseline':'Q5 samotné', 'opus-raw':'OPUS celé', 'opus-split':'OPUS po větách',
         'quickmt-raw':'QuickMT celé', 'quickmt-split':'QuickMT po větách',
         'quickmt-fast':'QuickMT po větách · beam 1', 'quickmt-q5':'QuickMT po větách → Q5'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root',type=Path)
    root=parser.parse_args().root;run=root/'quickmt-v1'
    def read(name):return json.loads((root/name).read_text(encoding='utf-8'))
    manifest=read('quickmt-v1/manifest.json');review=read('quickmt-review-notes.json')
    assert review['reviewed_through']==300 and (run/'COMPLETE.json').exists()
    assert hashlib.sha256(Path(__file__).with_name('evaluate_quickmt_translation.py').read_bytes()).hexdigest()==manifest['runner_sha256']
    assert hashlib.sha256(Path(__file__).with_name('subtitle_hybrid.py').read_bytes()).hexdigest()==manifest['hybrid_prompt_sha256']
    assert hashlib.sha256((run/'corpus.jsonl').read_bytes()).hexdigest()==manifest['corpus_sha256']
    prior={}
    for filename in ('comparison.json','hybrid-comparison.json'):
        for c in read(filename):
            for o in c['outputs'].values():prior[(c['en'],o['translation'])]=o['score']
    rows={}
    for s in (run/'results.jsonl').read_text(encoding='utf-8').splitlines():
        r=json.loads(s);key=(r['id'],r['profile']);assert key not in rows;rows[key]=r
    assert len(rows)==2100
    cases=[];methods=Counter()
    for c in [json.loads(s) for s in (run/'corpus.jsonl').read_text(encoding='utf-8').splitlines()]:
        outputs={}
        for p in PROFILES:
            r=rows[c['id'],p];old=prior.get((c['en'],r['translation']))
            override=review['overrides'].get(p,{}).get(str(int(c['id'])))
            score=override if override is not None else old if old is not None else 4
            method='explicit_review' if override is not None else 'exact_prior_review' if old is not None else 'new_faithful_review'
            if r['invalid']:score=0;method='invalid_response'
            outputs[p]={k:r[k] for k in ('translation','source_parts','translation_parts','cpu_draft','prompt','invalid','split_fallback')}
            outputs[p].update(score=score,method=method,total_ms=round(r['wall_ms'],2),
                cpu_ms=round(r['cpu_ms'],2),prep_ms=round(r['prep_ms'],2),gpu_api_ms=round(r['gpu_api_ms'],2))
        scores={}
        for o in outputs.values():scores[o['translation']]=min(scores.get(o['translation'],4),o['score'])
        for o in outputs.values():
            if o['score']!=scores[o['translation']]:o.update(score=scores[o['translation']],method='identical_current_output_review')
            methods[o['method']]+=1
        cases.append(dict(id=c['id'],en=c['en'],reference=c['reference'],category=c['category'],outputs=outputs,
            note=review['notes'].get(c['id'],''),split_count=len(outputs['quickmt-split']['source_parts'])))
    def med(values):return round(statistics.median(values),2)
    stats=[]
    for p in PROFILES:
        group=[c['outputs'][p] for c in cases];times=sorted(o['total_ms'] for o in group)
        stats.append(dict(profile=p,name=NAMES[p],count=len(group),acceptable=sum(o['score']>=3 for o in group),
            material_errors=sum(o['score']<=2 for o in group),critical_errors=sum(o['score']<=1 for o in group),
            median_ms=med(times),p95_ms=times[int(.95*(len(times)-1))],mean_ms=round(statistics.mean(times),2),
            cpu_median_ms=med([o['cpu_ms'] for o in group]),q5_api_median_ms=med([o['gpu_api_ms'] for o in group]),
            invalid=sum(o['invalid'] for o in group),split_fallbacks=sum(o['split_fallback'] for o in group)))
    def pair(left,right,subset):
        return dict(left=left,right=right,cases=len(subset),
            fixed=[c['id'] for c in subset if c['outputs'][left]['score']<=2<c['outputs'][right]['score']],
            introduced=[c['id'] for c in subset if c['outputs'][right]['score']<=2<c['outputs'][left]['score']],
            same_text=sum(c['outputs'][left]['translation']==c['outputs'][right]['translation'] for c in subset),
            median_delta_ms=med([c['outputs'][right]['total_ms']-c['outputs'][left]['total_ms'] for c in subset]))
    pairs=[pair('baseline',p,cases) for p in PROFILES[1:]]
    pairs.extend(pair(l,r,cases) for l,r in [('opus-raw','opus-split'),('quickmt-raw','quickmt-split'),
        ('quickmt-split','quickmt-fast'),('quickmt-split','quickmt-q5')])
    multi=[c for c in cases if c['split_count']>1]
    summary=dict(completed_utc=datetime.now(timezone.utc).isoformat(),cases=300,translations=2100,
        statistics=stats,paired=pairs,multi_sentence_count=len(multi),
        multi_sentence_pairs=[pair(l,r,multi) for l,r in [('opus-raw','opus-split'),('quickmt-raw','quickmt-split')]],
        rating_sources=dict(methods),production_changed=False,manifest=manifest,
        limits=['Known development set, not unseen validation. Assistant semantic review, not independent human accuracy.',
            'One warmed pass, alternating profile order; shared PC/GPU may affect latency. No claim about FPS improvement.',
            'Latency excludes OCR, subtitle display, model loading, downloads. Both CPU models use two threads, int8.',
            'QuickMT public 1.35s benchmark is a 1012-sentence GPU/batch benchmark, not this CPU per-caption latency.',
            'pySBD common splitter used here differs from QuickMT wrapper BlingFire.',
            'The hybrid uses the earlier frozen draft prompt; results do not cover every possible post-editing template.'])
    hardware=run/'hardware.json'
    if hardware.exists():
        summary['hardware']=json.loads(hardware.read_text(encoding='utf-8-sig'))
    for name,data in [('quickmt-comparison.json',cases),('quickmt-summary.json',summary)]:
        (root/name).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    e=html.escape
    table=''.join(f"<tr><th>{e(s['name'])}</th><td>{s['acceptable']}/300</td><td>{s['material_errors']}</td><td>{s['critical_errors']}</td><td>{s['median_ms']:.1f} ms</td><td>{s['p95_ms']:.1f} ms</td></tr>" for s in stats)
    paired=''.join(f"<li>{e(NAMES[p['left']])} → <b>{e(NAMES[p['right']])}</b>: opraveno {len(p['fixed'])}, přidáno {len(p['introduced'])} chyb; medián párové změny času {p['median_delta_ms']:+.1f} ms.</li>" for p in pairs[6:])
    page=TEMPLATE.replace('__TABLE__',table).replace('__PAIRED__',paired).replace('__MULTI__',str(len(multi)))
    by={s['profile']:s for s in stats}
    verdict=(f"Samotné Q5: {by['baseline']['material_errors']} významových chyb / 300. "
        f"QuickMT po větách: {by['quickmt-split']['material_errors']}; s následnou opravou Q5: {by['quickmt-q5']['material_errors']}. "
        f"U stávajícího OPUS dělení vět snížilo chyby z {by['opus-raw']['material_errors']} na {by['opus-split']['material_errors']}. "
        "Tento běh nepodporuje nahrazení výchozího Q5 novým hybridem.")
    page=page.replace('__VERDICT__',e(verdict))
    page=page.replace('__DATA__',json.dumps(cases,ensure_ascii=False).replace('</','<\\/')).replace('__NAMES__',json.dumps(NAMES,ensure_ascii=False))
    (root/'quickmt-report.html').write_text(page,encoding='utf-8')
    print(json.dumps({k:summary[k] for k in ('statistics','paired','multi_sentence_count','rating_sources')},ensure_ascii=False,indent=2))


TEMPLATE=r'''<!doctype html><html lang="cs"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FrameLab — QuickMT / OPUS / Q5</title><style>
:root{color-scheme:dark;font-family:Segoe UI,Arial,sans-serif;background:#10150f;color:#eef2e9}body{max-width:1300px;margin:auto;padding:28px 24px}h1{font-size:clamp(28px,4vw,44px)}p,li{line-height:1.65}a,.accent{color:#b6ff59}.muted,small{color:#b4c1a9}article,.panel{background:#1b2418;border:1px solid #394a30;border-radius:12px;padding:20px;margin:20px 0}table{border-collapse:collapse;width:100%;white-space:nowrap}th,td{text-align:left;padding:12px;border-bottom:1px solid #394a30}.scroll{overflow:auto}.controls{display:flex;gap:10px;flex-wrap:wrap;position:sticky;top:0;background:#10150ff5;padding:15px 0}input,select,button{font:inherit;background:#26321f;color:#edf2e5;border:1px solid #667b53;border-radius:6px;padding:10px}input{flex:1;min-width:180px}label{display:block}button,summary{cursor:pointer}.pair{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:20px}.pair section{background:#11190e;padding:16px;border-radius:8px}pre{white-space:pre-wrap;overflow-wrap:anywhere;font-size:13px}summary{padding:10px 0}.good{color:#84d7b4}.bad{color:#ff9b8d}small{display:block;margin:10px 0}.score{font-weight:bold}h3{line-height:1.5;font-size:19px}@media(max-width:750px){.pair{grid-template-columns:1fr}.controls{position:static}body{padding:18px 12px}}
</style></head><body><a href="/">← Průběh testů</a> · <a href="/hybrid-report.html">Předchozí slovník / CPU návrh</a><p class="accent">FRAME / LAB · LOKÁLNÍ EXPERIMENT</p><h1>QuickMT, OPUS a Q5: co pomůže titulkům?</h1><p>300 známých vět · 7 variant · 2 100 překladů · hotová inference i kontrola významu. CPU: Ryzen 9 5900X (test 2 thready); Q5: RTX 5070 Ti.</p>
<div class="panel"><b>Aplikace a její výchozí překladač zůstávají beze změny.</b><p>__VERDICT__</p><p>Porovnáváme model, dělení vět, rychlejší dekódování a dodatečnou opravu Q5 odděleně. U __MULTI__ vstupů rozdělil pySBD text na více částí. Rozdíl v rychlosti není automaticky důkaz lepšího překladu.</p><ul>__PAIRED__</ul></div>
<h2>Kvalita a čas celého překladu</h2><div class="scroll"><table><thead><tr><th>Varianta</th><th>Přijatelné</th><th>Významové chyby</th><th>Z toho kritické</th><th>Medián</th><th>P95</th></tr></thead><tbody>__TABLE__</tbody></table></div><p class="muted">Skóre 3–4 = zachovaný význam; 0–2 = podstatná chyba, z toho 0–1 kritická / neplatná. Jde o sémantickou kontrolu asistenta, nikoli nezávislý zaslepený lidský benchmark. Zcela stejné dříve hodnocené výstupy přebírají hodnocení; nové výstupy byly přečteny proti originálu. Reference není jediný přípustný překlad.</p>
<details class="panel"><summary>Metodika, nastavení a omezení</summary><ul><li>Původní známá vývojová sada 300, nikoli nové neviděné ověření. Žádné reference modelům. Po zahájení běhu žádné změny nastavení.</li><li>OPUS: stávající CPU model aplikace, CTranslate2 int8 (runtime int8_float32), 2 CPU thready, beam 2, repetition penalty 1.1, maximum 192 výstupních tokenů.</li><li>QuickMT EN–CS: revize da8d48f5a57bf942411ea9cf7b32ea6995a043c4, CC-BY-4.0, model.bin 409 915 789 B; CTranslate2 CPU int8 (runtime int8_float32), 2 thready, beam 2 nebo 1, repetition penalty 1, maximum 256 tokenů. Všechny stažené soubory ověřeny proti publikovaným hashům.</li><li>Společné dělení pySBD 0.3.4 bez čištění textu. Zkratky a desetinná čísla chrání pravidla; každý neprázdný znak originálu se musí zachovat, jinak se překládá celý text. Toto není přesná konfigurace wrapperu QuickMT, který používá BlingFire.</li><li>Jednotlivé věty jednoho titulku se překládají v batchi; různé titulky se nikdy neslučují. Výsledky částí se spojí mezerou.</li><li>Q5: CUNI-MH Q5_K_M na RTX 5070 Ti, celá původní šablona. Hybrid dostane celý originál a nový návrh QuickMT po větách, používá stejný zmrazený draft prompt jako předchozí test. Není to obecný chatovací korektor.</li><li>Měřeno včetně dělení, tokenizace, CPU a případného Q5 API. OCR, vykreslení a načtení modelů jsou mimo časy. Zahřátý jediný průchod; pořadí variant rotuje, PC/GPU sdílené s jinými procesy. Nejde o garantovanou odezvu během hraní.</li><li>Publikované QuickMT číslo 1,35 s je jiný GPU benchmark celého souboru 1 012 vět v batchích. Zde měříme CPU a jednotlivé titulky.</li></ul><p><a href="https://huggingface.co/quickmt/quickmt-en-cs">QuickMT model a licence</a> · <a href="https://github.com/quickmt/quickmt">Oficiální wrapper</a> · <a href="https://github.com/nipunsadvilkar/pySBD">pySBD</a> · <a href="/quickmt-summary.json">Úplné parametry, hashe a statistiky JSON</a></p></details>
<h2>Porovnání každé věty</h2><div class="controls"><label>Vlevo <select id="left"></select></label><label>Vpravo <select id="right"></select></label><label>Filtr <select id="filter"><option value="all">Všechny</option><option value="errors">Chyba v jedné z variant</option><option value="fixed">Vpravo opravená chyba</option><option value="worse">Vpravo nová chyba</option><option value="multi">Více částí po dělení</option></select></label><input id="search" placeholder="Hledat text nebo číslo" aria-label="Hledat"></div><p id="count" class="muted"></p><div id="cases"></div><button id="more">Dalších 30 vět</button>
<script id="data" type="application/json">__DATA__</script><script>
const data=JSON.parse(document.querySelector('#data').textContent),names=__NAMES__,left=document.querySelector('#left'),right=document.querySelector('#right'),filter=document.querySelector('#filter'),search=document.querySelector('#search'),list=document.querySelector('#cases'),more=document.querySelector('#more');let limit=30;
function el(tag,text,cls){const n=document.createElement(tag);if(text!==undefined)n.textContent=text;if(cls)n.className=cls;return n;}
for(const s of [left,right])for(const [k,v] of Object.entries(names)){const o=el('option',v);o.value=k;s.append(o);}left.value='baseline';right.value='quickmt-split';
function render(){const q=search.value.toLocaleLowerCase('cs'),rows=data.filter(c=>{const a=c.outputs[left.value],b=c.outputs[right.value];if(filter.value==='errors'&&![a,b].some(x=>x.score<=2))return false;if(filter.value==='fixed'&&!(a.score<=2&&b.score>=3))return false;if(filter.value==='worse'&&!(a.score>=3&&b.score<=2))return false;if(filter.value==='multi'&&c.split_count<2)return false;return [c.id,c.en,c.reference,c.note,a.translation,b.translation].join(' ').toLocaleLowerCase('cs').includes(q);});list.replaceChildren();document.querySelector('#count').textContent=rows.length+' vět · zobrazeno '+Math.min(limit,rows.length);for(const c of rows.slice(0,limit)){const card=el('article');card.append(el('small',c.id+' · '+c.category+' · '+c.split_count+' částí'),el('h3',c.en),el('p','Reference: '+c.reference,'muted'));const pair=el('div',undefined,'pair');for(const p of [left.value,right.value]){const o=c.outputs[p],s=el('section');s.append(el('b',names[p]),el('p',o.translation),el('p','Skóre '+o.score+'/4','score '+(o.score<=2?'bad':'good')),el('small',o.total_ms+' ms celkem · '+o.cpu_ms+' ms CPU · '+o.gpu_api_ms+' ms Q5 API'));if(o.translation_parts.length){const d=el('details');d.append(el('summary','Jednotlivé vstupní věty a CPU překlady'));for(let i=0;i<o.source_parts.length;i++)d.append(el('p',o.source_parts[i]),el('p',o.translation_parts[i]));s.append(d);}if(o.prompt){const d=el('details');d.append(el('summary','Přesný prompt Q5'),el('pre',o.prompt));s.append(d);}pair.append(s);}card.append(pair);if(c.note)card.append(el('p',c.note,'muted'));list.append(card);}more.hidden=limit>=rows.length;}
for(const c of [left,right,filter,search])c.addEventListener('input',()=>{limit=30;render();});more.addEventListener('click',()=>{limit+=30;render();});render();
</script></body></html>'''


if __name__=='__main__':main()
