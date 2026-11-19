"""Build local hybrid study report from saved outputs and explicit review notes."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import html
import json
import statistics
from pathlib import Path

from subtitle_hybrid import PROFILES

NAMES = {'baseline':'Samotné Q5', 'freedict-v1':'FreeDict + Q5', 'cpu-draft-v1':'CPU návrh + Q5'}


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def median(values):
    return round(statistics.median(values),2)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root',type=Path)
    args=parser.parse_args();root=args.root;run=root/'hybrid-v1'
    manifest=read(run/'manifest.json');review=read(root/'hybrid-review-notes.json')
    assert review['reviewed_through']==300 and (run/'COMPLETE.json').exists()
    assert hashlib.sha256(Path(__file__).with_name('subtitle_hybrid.py').read_bytes()).hexdigest()==manifest['candidate_sha256']
    corpus=[json.loads(x) for x in (run/'corpus.jsonl').read_text(encoding='utf-8').splitlines()]
    assert hashlib.sha256((run/'corpus.jsonl').read_bytes()).hexdigest()==manifest['corpus_sha256']
    prior={}
    for c in read(root/'comparison.json'):
        for o in c['outputs'].values():
            key=(c['en'],o['translation'])
            assert key not in prior or prior[key]==o['score']
            prior[key]=o['score']
    results={}
    for line in (run/'results.jsonl').read_text(encoding='utf-8').splitlines():
        row=json.loads(line);key=(row['id'],row['profile'])
        assert key not in results
        results[key]=row
    assert len(results)==900
    cases=[];methods=Counter()
    for c in corpus:
        outputs={}
        for p in PROFILES:
            o=results[c['id'],p]
            old=prior.get((c['en'],o['translation']))
            explicit=review['overrides'][p].get(c['id'].lstrip('0') or '0')
            score=explicit if explicit is not None else old if old is not None else 4
            method='assistant_explicit_review' if explicit is not None else 'identical_previously_reviewed_output' if old is not None else 'assistant_review_faithful_paraphrase'
            if o['invalid']:score=0;method='invalid_response'
            methods[method]+=1
            outputs[p]=dict(translation=o['translation'],score=score,method=method,total_ms=round(o['wall_ms'],2),
                prep_ms=round(o['prep_ms'],3),gpu_api_ms=round(o['gpu_api_ms'],2),cpu_draft=o['cpu_draft'],
                hints=o['hints'],prompt=o['prompt'],invalid=o['invalid'])
        # Review is semantic, not profile-specific: identical outputs must agree.
        consistency={}
        for o in outputs.values():consistency[o['translation']]=min(consistency.get(o['translation'],4),o['score'])
        for o in outputs.values():
            if o['score']!=consistency[o['translation']]:
                o['score']=consistency[o['translation']];o['method']+='; identical-output consistency'
        cases.append(dict(id=c['id'],en=c['en'],reference=c['reference'],category=c['category'],outputs=outputs,
            note=review['notes'].get(c['id'],'')))
    stats=[]
    for p in PROFILES:
        rows=[c['outputs'][p] for c in cases]
        times=sorted(r['total_ms'] for r in rows)
        stats.append(dict(profile=p,name=NAMES[p],count=len(rows),acceptable=sum(r['score']>=3 for r in rows),
            material_errors=sum(r['score']<=2 for r in rows),critical_errors=sum(r['score']<=1 for r in rows),
            mean_score=round(statistics.mean(r['score'] for r in rows),3),median_ms=median(times),
            p95_ms=times[int(.95*(len(times)-1))],mean_ms=round(statistics.mean(times),2),
            prep_median_ms=median([r['prep_ms'] for r in rows]),gpu_api_median_ms=median([r['gpu_api_ms'] for r in rows]),
            invalid_api_responses=sum(r['invalid'] for r in rows)))
    pairs=[]
    for p in PROFILES[1:]:
        pairs.append(dict(profile=p,fixed=[c['id'] for c in cases if c['outputs']['baseline']['score']<=2<c['outputs'][p]['score']],
            introduced=[c['id'] for c in cases if c['outputs'][p]['score']<=2<c['outputs']['baseline']['score']],
            same_text=sum(c['outputs'][p]['translation']==c['outputs']['baseline']['translation'] for c in cases),
            median_paired_total_difference_ms=median([c['outputs'][p]['total_ms']-c['outputs']['baseline']['total_ms'] for c in cases])))
    summary=dict(completed_utc=datetime.now(timezone.utc).isoformat(),cases=300,translations=900,statistics=stats,paired=pairs,
        dictionary_coverage=sum(bool(c['outputs']['freedict-v1']['hints']) for c in cases),
        cpu_draft_copied_exactly=sum(c['outputs']['cpu-draft-v1']['translation']==c['outputs']['cpu-draft-v1']['cpu_draft'] for c in cases),
        rating_sources=dict(methods),production_changed=False,manifest=manifest,
        limits=['Known development set, not unseen validation.','Assistant review, not independent blinded human accuracy.',
            'One warmed sequential pass, balanced profile order; concurrent live GPU work and caching may affect timings.',
            'Wall time includes CPU draft/lookup + Q5; excludes OCR, caption display, model/dictionary startup.',
            'Results apply to these exact lookup/template/CPU settings, not every hybrid translation system.',
            'CPU proposal uses neural OPUS-MT; only the dictionary lookup is non-neural.'],
        decision='Keep current production Q5 unchanged; neither hybrid is promoted automatically.')
    for name,data in [('hybrid-comparison.json',cases),('hybrid-summary.json',summary)]:
        (root/name).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    e=html.escape
    table=''.join(f"<tr><th>{e(s['name'])}</th><td>{s['acceptable']}/300</td><td>{s['material_errors']}</td><td>{s['critical_errors']}</td><td>{s['median_ms']:.1f} ms</td><td>{s['p95_ms']:.1f} ms</td><td>{s['prep_median_ms']:.2f} ms</td></tr>" for s in stats)
    findings=''.join(f"<li><b>{e(NAMES[p['profile']])}</b>: opravilo {len(p['fixed'])} významových chyb, přidalo {len(p['introduced'])} nových. Medián párového rozdílu celkového času {p['median_paired_total_difference_ms']:+.1f} ms.</li>" for p in pairs)
    data=json.dumps(cases,ensure_ascii=False).replace('</','<\\/')
    page=TEMPLATE.replace('__TABLE__',table).replace('__FINDINGS__',findings).replace('__DATA__',data)
    page=page.replace('__COVERAGE__',str(summary['dictionary_coverage'])).replace('__CPU_LOAD__',f"{manifest['cpu_load_ms']:.0f}").replace('__DICT_LOAD__',f"{manifest['dictionary_load_ms']:.0f}")
    (root/'hybrid-report.html').write_text(page,encoding='utf-8')
    print(json.dumps({k:summary[k] for k in ['statistics','paired','dictionary_coverage','cpu_draft_copied_exactly','rating_sources']},ensure_ascii=False,indent=2))


TEMPLATE=r'''<!doctype html><html lang="cs"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FrameLab — Q5 / slovník / CPU návrh</title><style>
:root{color-scheme:dark;font-family:Segoe UI,Arial,sans-serif;background:#10150f;color:#eef2e9}body{max-width:1350px;margin:auto;padding:28px 24px}h1{font-size:clamp(27px,4vw,43px);margin:14px 0}h2{margin-top:30px}p,li{line-height:1.6}a,.accent{color:#b6ff59}.muted,small{color:#acbaa2}.panel,article{background:#1b2418;border:1px solid #394a30;border-radius:12px;padding:20px;margin:18px 0}table{width:100%;border-collapse:collapse;white-space:nowrap}th,td{text-align:left;padding:12px;border-bottom:1px solid #394a30}.scroll{overflow:auto}.controls{display:flex;gap:12px;flex-wrap:wrap;position:sticky;top:0;background:#10150ff5;padding:12px 0}input,select,button{font:inherit;background:#26321f;color:#edf2e5;border:1px solid #667b53;border-radius:6px;padding:10px}input{flex:1;min-width:220px}button{cursor:pointer}.triple{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:18px}.triple section{background:#11190e;padding:16px;border-radius:8px}pre{white-space:pre-wrap;overflow-wrap:anywhere;font-size:13px}summary{cursor:pointer;padding:10px 0}.good{color:#84d7b4}.bad{color:#ff9b8d}.ref{color:#b9c8ac}.badge{font-size:13px;display:block;margin-top:10px}h3{line-height:1.5;font-size:19px}label{display:block;font-weight:600}.counter{margin:14px 0}@media(max-width:850px){.triple{grid-template-columns:1fr}body{padding:18px 12px}.controls{position:static}}
</style></head><body><a href="/">← Sledování průběhu</a> · <a href="/report.html">Předchozí ladění promptů</a><p class="accent">FRAME / LAB · LOKÁLNÍ EXPERIMENT</p><h1>Pomůže Q5 slovník nebo CPU návrh?</h1><p>300 stejných vět · 3 varianty · 900 překladů · NVIDIA RTX 5070 Ti · CUNI-MH Q5_K_M</p>
<div class="panel"><b>Test i hodnocení jsou dokončené. Výchozí aplikace zůstala beze změny.</b><p>V tomto pokusu oba hybridy přinesly nové významové chyby. Místo samotného Q5 je proto nenasazuji. Jde o výsledek konkrétních šablon a nastavení, nikoli důkaz, že žádný hybridní překladač nemůže fungovat.</p><ul>__FINDINGS__</ul></div>
<h2>Kvalita a celkový čas</h2><div class="scroll"><table><thead><tr><th>Varianta</th><th>Přijatelné</th><th>Významové chyby</th><th>Z toho kritické / neplatné</th><th>Medián celkem</th><th>P95 celkem</th><th>Medián přípravy</th></tr></thead><tbody>__TABLE__</tbody></table></div>
<p class="muted">Hodnocení 0–4 provedl asistent, nejde o nezávislý zaslepený lidský benchmark. 3–4 = význam zachován, 0–2 = podstatná chyba; kritické / neplatné 0–1 jsou podmnožina. Nové výstupy byly přečteny proti originálu, přesně stejné již hodnocené výstupy přebírají skóre. Referenční překlad je pomůcka; správná parafráze nemusí být doslovně stejná.</p>
<details class="panel"><summary>Co přesně se testovalo a co znamenají časy</summary><ul><li>Samotné Q5: původní produkční šablona a stejné parametry generování.</li><li>FreeDict + Q5: publikovaný EN–CS slovník 0.1.3, 150 004 záznamů / 81 188 unikátních hesel. Přesné fráze do čtyř slov, přednost delším frázím, pak delším obsahovým slovům. Nejvýše osm hesel, čtyři významy na heslo, 800 znaků nápovědy; běžná funkční slova vynechána. Heslo se našlo u __COVERAGE__/300 vět. Žádné ruční opravy hesel podle referencí.</li><li>CPU návrh + Q5: lokální OPUS-MT EN–CS, CTranslate2 int8, dva CPU thready, beam 2, stejné parametry jako CPU backend aplikace. Q5 dostalo celý originál a celý CPU návrh. OPUS-MT je neuronový model, pouze slovníkové vyhledávání je ne-neuronové.</li><li>Celkem = tvorba promptu a vyhledání hesel / CPU překlad + Q5 API. Zahřáté modely. Načtení slovníku (__DICT_LOAD__ ms) a CPU modelu (__CPU_LOAD__ ms) je zvlášť, mimo průběžné časy. OCR, vykreslení titulků a displej se neměří.</li><li>Pořadí variant se střídalo ve všech šesti permutacích, každá 50×. Jeden průchod; běžící FrameLab mohl sdílet GPU. Výsledky nejsou izolovaný hardwarový benchmark.</li><li>Původních 300 známých vývojových vět, nikoli nová testovací sada. Žádná reference nebyla poslána modelu. Během běhu se šablony ani slovník neupravovaly.</li><li>Pravidla hodnocení: 4 věrný a přirozený, 3 drobná jazyková chyba, 2 podstatná místní chyba / vynechání / doplnění, 1 kriticky změněný význam, 0 neplatný nebo nepřeložený text. U věty 166 byl i baseline nově posouzen kvůli vynechání NASDA.</li></ul><p>Zdroje: <a href="https://freedict.org/downloads/">FreeDict</a> (GPL-2.0-or-later, data pouze lokálně), <a href="https://huggingface.co/Helsinki-NLP/opus-mt-en-cs">OPUS-MT</a>, <a href="https://huggingface.co/wmt24-cuni/CUNI-MH/blob/main/README.md">CUNI-MH a požadovaná šablona</a>. <a href="/hybrid-summary.json">Souhrn JSON včetně parametrů a hashů</a>.</p></details>
<h2>Každá věta vedle sebe</h2><div class="controls"><select id="filter" aria-label="Filtr"><option value="all">Všechny věty</option><option value="error">Alespoň jedna významová chyba</option><option value="dictbetter">Slovník opravil chybu</option><option value="dictworse">Slovník přidal chybu</option><option value="cpubetter">CPU návrh opravil chybu</option><option value="cpuworse">CPU návrh přidal chybu</option></select><input id="search" placeholder="Hledat v originálu, překladu, CPU návrhu…" aria-label="Hledat"></div><div id="count" class="counter muted"></div><div id="cases"></div><button id="more">Dalších 30 vět</button>
<script id="data" type="application/json">__DATA__</script><script>
const data=JSON.parse(document.querySelector('#data').textContent),names={'baseline':'Samotné Q5','freedict-v1':'FreeDict + Q5','cpu-draft-v1':'CPU návrh + Q5'},filter=document.querySelector('#filter'),search=document.querySelector('#search'),list=document.querySelector('#cases'),more=document.querySelector('#more');let limit=30;
function el(tag,text,cls){const n=document.createElement(tag);if(text!==undefined)n.textContent=text;if(cls)n.className=cls;return n;}
function render(){const q=search.value.toLocaleLowerCase('cs');const rows=data.filter(c=>{const b=c.outputs.baseline,d=c.outputs['freedict-v1'],p=c.outputs['cpu-draft-v1'];if(filter.value==='error'&&![b,d,p].some(x=>x.score<=2))return false;if(filter.value==='dictbetter'&&!(b.score<=2&&d.score>=3))return false;if(filter.value==='dictworse'&&!(b.score>=3&&d.score<=2))return false;if(filter.value==='cpubetter'&&!(b.score<=2&&p.score>=3))return false;if(filter.value==='cpuworse'&&!(b.score>=3&&p.score<=2))return false;return [c.id,c.en,c.reference,c.note,...Object.values(c.outputs).map(x=>x.translation),p.cpu_draft].join(' ').toLocaleLowerCase('cs').includes(q);});list.replaceChildren();document.querySelector('#count').textContent=rows.length+' vět · zobrazeno '+Math.min(limit,rows.length);for(const c of rows.slice(0,limit)){const card=el('article');card.append(el('small',c.id+' · '+c.category),el('h3',c.en),el('p','Reference: '+c.reference,'ref'));const triple=el('div',undefined,'triple');for(const [key,v] of Object.entries(c.outputs)){const s=el('section');s.append(el('label',names[key]),el('p',v.translation),el('span','Hodnocení '+v.score+'/4',v.score<=2?'badge bad':'badge good'),el('small',v.total_ms+' ms celkem · '+v.prep_ms+' ms příprava · '+v.gpu_api_ms+' ms Q5'));if(v.cpu_draft){const d=el('details');d.append(el('summary','Návrh z CPU před opravou'),el('p',v.cpu_draft));s.append(d);}if(v.hints.length){const d=el('details');d.append(el('summary','Použitá slovníková hesla'),el('pre',v.hints.map(x=>x.line).join('\n')));s.append(d);}const d=el('details');d.append(el('summary','Přesný prompt'),el('pre',v.prompt));s.append(d);triple.append(s);}card.append(triple);if(c.note)card.append(el('p',c.note,'muted'));list.append(card);}more.hidden=limit>=rows.length;}
[filter,search].forEach(x=>x.addEventListener('input',()=>{limit=30;render();}));more.addEventListener('click',()=>{limit+=30;render();});render();
</script></body></html>'''


if __name__=='__main__':main()
