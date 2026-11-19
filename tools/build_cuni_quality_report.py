"""Build a portable, offline quality-only report. Never deploy or invent grades."""
from pathlib import Path
import argparse, collections, csv, hashlib, json, shutil


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--results',type=Path,required=True)
    p.add_argument('--review',type=Path,required=True)
    p.add_argument('--historical',type=Path,required=True)
    p.add_argument('--hardware',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    notes=json.loads(a.review.read_text(encoding='utf-8'))
    reviewed={i for first,last in notes['reviewed_ranges'] for i in range(first,last+1)}
    assert reviewed==set(range(1,301)), 'All cases must have been reviewed'
    old=json.loads((a.historical/'porovnani-300.json').read_text(encoding='utf-8'))
    q={k:{x['id']:x for x in map(json.loads,(a.results/(v+'.jsonl')).read_text(encoding='utf-8').splitlines())} for k,v in [('q3','cuni-q3km'),('q8','cuni-q8')]}
    assert len(old)==len(q['q3'])==len(q['q8'])==300
    rows=[]
    for x in old:
        overrides=notes['overrides'].get(x['id'],{})
        row={k:x[k] for k in ['id','source','en','reference']}
        for model in ['q3','q8']:
            row[model]=q[model][x['id']]['translation']
            row[model+'_score']=overrides.get(model,x['cuni_score'])
            assert 0<=row[model+'_score']<=4
        row['review_note']=overrides.get('note','Reviewed against the English source; prior CUNI grading remains appropriate for both outputs.')
        row['historical_q3_note']=x['cuni_note']
        row['historical_q3_note_context']='Prior laptop CUNI vs Hy-MT2 study. Mentions of Q8 in this historical note refer to Hy-MT2, NOT the current CUNI Q8.'
        rows.append(row)
    summary={}
    for model in ['q3','q8']:
        scores=[r[model+'_score'] for r in rows]
        summary[model]=dict(accepted=sum(s>=3 for s in scores),major=sum(s<=1 for s in scores),mean=sum(scores)/300,counts=dict(collections.Counter(scores)))
    summary.update(cases=300,identical=sum(r['q3']==r['q8'] for r in rows),q8_higher=sum(r['q8_score']>r['q3_score'] for r in rows),q8_lower=sum(r['q8_score']<r['q3_score'] for r in rows),method=notes['method'],performance_measured=False)
    data=a.output/'data';data.mkdir(exist_ok=True)
    for name,value in [('comparison.json',rows),('summary.json',summary),('review.json',notes)]:
        (data/name).write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')
    for model in ['cuni-q3km','cuni-q8']:
        shutil.copy2(a.results/(model+'.jsonl'),data/(model+'.jsonl'))
    shutil.copy2(a.results/'manifest.json',data/'manifest.json')
    shutil.copy2(Path(__file__).resolve().parents[1]/'docs/subtitle-models.json',data/'model-catalog.json')
    shutil.copy2(a.hardware,data/'hardware.json')
    shutil.copy2(a.historical/'corpus.jsonl',data/'corpus.jsonl')
    shutil.copytree(a.historical/'sources',a.output/'sources',dirs_exist_ok=True)
    with (data/'comparison.csv').open('w',encoding='utf-8-sig',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    payload=json.dumps(dict(rows=rows,summary=summary),ensure_ascii=False).replace('</','<\\/')
    html=TEMPLATE.replace('__DATA__',payload)
    (a.output/'report.html').write_text(html,encoding='utf-8')
    (a.output/'README.txt').write_text('CUNI-MH Q3_K_M versus CUNI-MH Q8_0, 300 fixed sentences.\nOpen report.html offline. This is TEXT QUALITY ONLY. No concurrent OCR/VFG/NR performance claim.\nRatings are assistant judgments, not blinded independent human evaluation.\nRaw response timings include local runtime effects and must not be used as a coexistence benchmark.\nOriginal game text/licenses and original model cards/provenance are retained under sources.\nThe prior laptop report used a DIFFERENT Q8 model (Hy-MT2); do not confuse the studies.\nNo model weights or DLLs in this report.\n',encoding='utf-8')
    records=[dict(file=f.relative_to(a.output).as_posix(),bytes=f.stat().st_size,sha256=hashlib.sha256(f.read_bytes()).hexdigest()) for f in sorted(a.output.rglob('*')) if f.is_file() and f.name!='files-sha256.json']
    (a.output/'files-sha256.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
    print(json.dumps(summary,indent=2))


TEMPLATE=r'''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FrameLab · CUNI Q3 vs Q8 · 300 sentences</title>
<style>
:root{color-scheme:dark;--bg:#101511;--panel:#1a2119;--line:#394331;--ink:#eef0e7;--muted:#a7afa0;--lime:#c1ff5e;--orange:#ff8659}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:17px/1.6 system-ui,sans-serif}main{max-width:1240px;margin:auto;padding:32px 24px 70px}a{color:var(--lime)}header{display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid var(--line);padding-bottom:20px}.brand{font-size:24px;font-weight:800;letter-spacing:-1px}.brand span{color:var(--lime)}button,input,select{font:inherit;color:inherit;background:var(--panel);border:1px solid var(--line);border-radius:5px;padding:10px 14px}button{cursor:pointer}button:focus-visible,a:focus-visible,input:focus-visible,select:focus-visible{outline:2px solid var(--lime);outline-offset:3px}h1{font-size:clamp(32px,5vw,62px);line-height:1.1;letter-spacing:-2px;max-width:850px;margin:40px 0 20px}h2{font-size:25px;line-height:1.25;margin:0 0 14px}p{max-width:82ch}.kicker,.meta{font:14px/1.5 ui-monospace,monospace;color:var(--muted)}.notice{border-left:3px solid var(--orange);padding:12px 18px;background:#231e16;max-width:none}.cards{display:grid;grid-template-columns:1fr 1fr;gap:18px;margin:32px 0}.card{border:1px solid var(--line);padding:24px;border-radius:8px;background:var(--panel)}.big{font-size:48px;font-weight:750;color:var(--lime);line-height:1.2}.card small{font-size:16px;color:var(--muted)}.details{display:flex;gap:24px;flex-wrap:wrap}.tools{display:flex;gap:12px;flex-wrap:wrap;margin:30px 0 12px}.tools input{flex:1;min-width:180px}.case{border-top:1px solid var(--line);padding:24px 0}.case h3{font-size:18px;margin:0 0 12px}.source,.reference{margin:8px 0}.reference{color:var(--muted)}.outputs{display:grid;grid-template-columns:1fr 1fr;gap:18px;margin:16px 0}.output{padding:18px;border:1px solid var(--line);border-radius:6px;overflow-wrap:anywhere}.output p{margin:10px 0 0}.score{float:right;font-weight:750}.bad{color:var(--orange)}.note{font-size:15px;color:var(--muted)}details summary{cursor:pointer}footer{border-top:1px solid var(--line);margin-top:40px;padding-top:22px;font-size:15px;color:var(--muted)}@media(max-width:650px){main{padding:20px 16px}.cards,.outputs{grid-template-columns:1fr}.big{font-size:38px}.tools>*{width:100%}h1{letter-spacing:-1px}}
</style><main><header><div class="brand">FRAME<span>/</span>LAB</div><button id="language" type="button">Čeština</button></header>
<p class="kicker">CUNI-MH 7B · RTX 5070 Ti · 2026-11-19</p><h1 id="title"></h1><p id="intro"></p><p class="notice" id="scope"></p>
<section class="cards" aria-label="Results"><article class="card"><h2>Q3_K_M</h2><div class="big" id="q3count"></div><small class="accepted"></small><p id="q3detail"></p><div class="meta">3.52 GB GGUF</div></article><article class="card"><h2>Q8_0</h2><div class="big" id="q8count"></div><small class="accepted"></small><p id="q8detail"></p><div class="meta">7.70 GB GGUF</div></article></section>
<p id="paired"></p><details><summary id="methodTitle"></summary><p id="method"></p><p id="rubric"></p><p id="hardware"></p><p id="history"></p></details>
<p class="details"><a href="data/comparison.csv">CSV</a><a href="data/comparison.json">JSON</a><a href="data/manifest.json">Prompt / settings</a><a href="data/review.json">Review notes</a><a href="data/hardware.json">Hardware</a><a href="files-sha256.json">SHA-256</a></p>
<div class="tools"><input id="search" type="search" aria-label="Search sentences"><select id="group" aria-label="Source"><option value="">All sources</option><option value="wesnoth">Wesnoth</option><option value="warzone">Warzone 2100</option><option value="authored">Diagnostic</option></select><select id="filter" aria-label="Comparison filter"></select></div><p class="meta" id="count" aria-live="polite"></p><section id="cases"></section>
<footer><p id="rights"></p><a href="https://github.com/tomik520i/FrameLab">GitHub</a> · <a href="https://opicebot.cz/framelab/">FrameLab website</a></footer></main>
<script id="dataset" type="application/json">__DATA__</script><script>
const data=JSON.parse(document.getElementById('dataset').textContent),el=id=>document.getElementById(id);let cs=new URLSearchParams(location.search).get('lang')==='cs';
const t=(en,cz)=>cs?cz:en;function node(tag,text,cls){const n=document.createElement(tag);n.textContent=text;if(cls)n.className=cls;return n}
function render(){document.documentElement.lang=cs?'cs':'en';el('language').textContent=cs?'English':'Čeština';el('title').textContent=t('More precision. Better Czech?','Více přesnosti. Lepší čeština?');el('intro').textContent=t('The same 300 sentences, the same CUNI-MH model, two quantizations. This sample does not show a clear quality win for Q8. Q3 remains the more economical starting point.','Stejných 300 vět, stejný model CUNI-MH, dvě kvantizace. Tento vzorek neukazuje jasnou kvalitativní výhodu Q8. Q3 zůstává úspornější výchozí volbou.');el('scope').textContent=t('TEXT QUALITY ONLY. These are exact English inputs, not OCR images. OCR, frame generation and the enhancer were not running concurrently in this preflight. GPU contention, live subtitle delay and monitor FPS are not measured here.','POUZE KVALITA TEXTOVÉHO PŘEKLADU. Vstupem jsou přesné anglické věty, nikoli snímky pro OCR. V tomto předběžném testu současně neběželo OCR, frame gen ani enhancer. Nejde o měření jejich souběhu, zpoždění živých titulků ani FPS monitoru.');
for(const m of ['q3','q8']){const s=data.summary[m];el(m+'count').textContent=s.accepted+' / 300';el(m+'detail').textContent=t(s.major+' major errors · mean '+s.mean.toFixed(2)+'/4','Zásadní chyby: '+s.major+' · průměr '+s.mean.toFixed(2)+'/4')}document.querySelectorAll('.accepted').forEach(x=>x.textContent=t('acceptable translations (score 3–4)','přijatelných překladů (skóre 3–4)'));
el('paired').textContent=t(`${data.summary.identical} outputs are identical. Q8 scored higher in ${data.summary.q8_higher} cases and lower in ${data.summary.q8_lower}; the remaining scores tie.`,`${data.summary.identical} výstupů je totožných. Q8 získal vyšší známku v ${data.summary.q8_higher} případech a nižší v ${data.summary.q8_lower}; ostatní známky jsou shodné.`);
el('methodTitle').textContent=t('Method and limits','Metoda a omezení');el('method').textContent=t('Assistant review, not a blinded independent human study. Every pair was checked against the English source and the pre-existing reference. Prior CUNI grades were retained where still appropriate; all changes are explicit in review.json. References were never provided to the model. The corpus has 100 Wesnoth, 100 Warzone 2100 and 100 authored diagnostic inputs.','Hodnotil asistent, nejde o slepý nezávislý lidský test. Každý pár byl zkontrolován proti anglickému zdroji a předem existující referenci. Dřívější známky CUNI zůstaly tam, kde byly stále odpovídající; změny jsou výslovně v review.json. Model reference neviděl. Korpus: 100 vět Wesnoth, 100 Warzone 2100 a 100 autorských diagnostických vět.');el('rubric').textContent=t('4: faithful and natural · 3: minor defect · 2: substantial error · 1: seriously wrong meaning/instruction · 0: empty/unrelated. These judgments are approximate, not certified translation accuracy.','4: věrný a přirozený · 3: drobná chyba · 2: podstatná chyba · 1: zásadně chybný význam/pokyn · 0: prázdný/nesouvisející. Jde o orientační úsudek, nikoli certifikovanou přesnost.');el('hardware').textContent='Ryzen 9 5900X · 64 GB RAM · RTX 5070 Ti 16 GB · Windows 11 Pro · Ollama 0.40.16 · 2048 context · temperature 0 · full GPU offload';el('history').textContent=t('Earlier laptop reports called a DIFFERENT model “Q8”: Hy-MT2 1.8B. Here both columns are CUNI-MH 7B. Fresh desktop Q3 differs from the archived laptop Q3 in 20 strings; hardware/runtime differences can affect greedy decoding.','Starší notebookový report označoval jako Q8 JINÝ model: Hy-MT2 1.8B. Zde jsou oba sloupce CUNI-MH 7B. Čerstvý desktopový Q3 se liší od archivovaného notebookového Q3 ve 20 textech; rozdíly hardwaru/runtime mohou ovlivnit i greedy dekódování.');el('rights').textContent=t('Game texts retain their original authorship and licenses under sources. No weights or DLLs are included. Raw responses and settings are under data; their timings are not a concurrent-performance benchmark.','Herní texty si zachovávají autorství a licence v sources. Balík neobsahuje váhy ani DLL. Surové odpovědi a nastavení jsou v data; jejich časy nejsou měřením souběžného výkonu.');el('search').placeholder=t('Search English or Czech text…','Hledat anglický nebo český text…');
const selected=el('filter').value;el('filter').replaceChildren();[['',t('All pairs','Všechny páry')],['better',t('Q8 scores higher','Q8 má vyšší známku')],['worse',t('Q8 scores lower','Q8 má nižší známku')],['major',t('At least one major error','Alespoň jedna zásadní chyba')],['different',t('Different text','Rozdílný text')]].forEach(([v,label])=>{const o=node('option',label);o.value=v;el('filter').append(o)});el('filter').value=selected;el('group').options[0].textContent=t('All sources','Všechny zdroje');show()}
function show(){const search=el('search').value.toLocaleLowerCase(),group=el('group').value,f=el('filter').value;const selected=data.rows.filter(r=>(!group||r.source===group)&&(!search||[r.id,r.en,r.reference,r.q3,r.q8].join(' ').toLocaleLowerCase().includes(search))&&(!f||(f==='better'&&r.q8_score>r.q3_score)||(f==='worse'&&r.q8_score<r.q3_score)||(f==='major'&&(r.q3_score<=1||r.q8_score<=1))||(f==='different'&&r.q3!==r.q8)));el('count').textContent=selected.length+' / 300';const fragment=document.createDocumentFragment();for(const r of selected){const a=node('article','', 'case');a.append(node('h3',r.id+' · '+r.source),node('p',r.en,'source'),node('p',t('Reference: ','Reference: ')+r.reference,'reference'));const outputs=node('div','','outputs');for(const [key,label] of [['q3','Q3_K_M'],['q8','Q8_0']]){const box=node('section','','output'),score=node('span',r[key+'_score']+'/4','score'+(r[key+'_score']<=1?' bad':''));box.append(score,node('strong',label),node('p',r[key]));outputs.append(box)}a.append(outputs);if(r.review_note.startsWith('Reviewed')){}else a.append(node('p',r.review_note,'note'));fragment.append(a)}el('cases').replaceChildren(fragment)}
el('language').addEventListener('click',()=>{cs=!cs;render()});el('search').addEventListener('input',show);el('group').addEventListener('change',show);el('filter').addEventListener('change',show);render();
</script></html>'''


if __name__=='__main__':main()
