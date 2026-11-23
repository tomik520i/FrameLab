"""Render saved, individually assigned Codex ratings. No inference or API access."""
from pathlib import Path
import argparse, csv, hashlib, html, json, shutil, statistics

ROOT = Path(__file__).resolve().parents[1]
LABELS = {'cpu': 'OPUS-MT / CPU', 'cuni-q3km': 'CUNI-MH 7B Q3_K_M',
          'translategemma-4b-q4km': 'TranslateGemma 4B Q4_K_M', 'gpt-6-luna': 'OpenAI GPT-6 Luna · none'}

def read(path):
    return [json.loads(x) for x in path.read_text('utf-8').splitlines() if x.strip()]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--review', type=Path, required=True)
    p.add_argument('--benchmark', type=Path, required=True)
    a = p.parse_args()
    out = ROOT / 'website/runs/2026-11-23-direct-review'
    data = out / 'data'; data.mkdir(parents=True, exist_ok=True)
    mapping = json.loads((a.review/'review-mapping.json').read_text('utf-8'))
    cases = read(a.review/'review-blind-input.jsonl')
    originals = read(a.benchmark/'dataset500.jsonl')
    rows = {m: {r['id']: r for r in read(a.benchmark/(m+'.jsonl'))} for m in LABELS}
    ratings = {}
    for line in (a.review/'direct-review-ratings.txt').read_text('utf-8').splitlines():
        if not line.strip() or line.startswith('#'): continue
        num, *values = line.split(); ident = 'w500-'+num
        assert ident not in ratings and len(values) == 4
        assert all(len(v) == 2 and set(v) <= set('01234') for v in values)
        ratings[ident] = dict(zip('ABCD', values))
    assert len(cases) == len(ratings) == len(mapping) == 500
    assert {r['id'] for r in originals} == set(ratings) == set(mapping)
    notes = {}
    for file in sorted(a.review.glob('direct-review-notes*.json')):
        part = json.loads(file.read_text('utf-8'))
        assert not set(part).intersection(notes), file
        notes.update(part)
    combined = []
    for c, original in zip(cases, originals):
        ident = c['id']; assert c['en'] == original['en'] and c['id'] == original['id']
        item = {k: c[k] for k in ('id', 'en', 'reference_cs')}
        item['note_cs'] = notes.get(ident, {}).get('_case', '')
        item['models'] = {}
        assert set(mapping[ident].values()) == set(LABELS)
        for label, model in mapping[ident].items():
            raw = rows[model][ident]; meaning, fluency = map(int, ratings[ident][label])
            score = (3*meaning+fluency)*100/16 if fluency else 0
            item['models'][model] = dict(translation=raw['translation'], error=raw.get('error',''),
                ms=raw['ms'], meaning=meaning, fluency=fluency, score=score,
                note_cs=notes.get(ident, {}).get(label,''), blind_label=label)
        scores = [r['score'] for r in item['models'].values()]
        item['spread'] = max(scores)-min(scores)
        combined.append(item)
    summaries = []
    for model, label in LABELS.items():
        values = [c['models'][model] for c in combined]
        times = sorted(v['ms'] for v in values)
        summaries.append(dict(model=model, label=label, score=statistics.mean(v['score'] for v in values),
            meaning=statistics.mean(v['meaning'] for v in values), fluency=statistics.mean(v['fluency'] for v in values),
            usable=sum(v['meaning']>=3 and v['fluency']>=3 for v in values),
            material=sum(v['meaning']<=2 for v in values), non_czech_or_missing=sum(v['fluency']==0 for v in values),
            perfect=sum(v['meaning']==4 and v['fluency']==4 for v in values),
            errors=sum(bool(v['error']) for v in values), median_ms=statistics.median(times), p95_ms=times[474],
            tied_best=sum(c['models'][model]['score']==max(v['score'] for v in c['models'].values()) for c in combined),
            unique_best=sum(c['models'][model]['score']>max(v['score'] for m,v in c['models'].items() if m!=model) for c in combined)))
    summaries.sort(key=lambda m: -m['score'])
    report = dict(reviewer='Codex assistant in the project conversation; not independent human validation',
        paid_api_requests_for_this_review=0, new_user_api_cost_usd=0,
        rubric={'meaning_weight':.75,'fluency_weight':.25,'scale':'0–4 each',
                'formula':'100 * (3 * meaning + fluency) / 16; score = 0 when fluency = 0',
                'usable':'meaning >= 3 and fluency >= 3', 'material_error':'meaning <= 2'},
        blinding={'seed':2026112401,'candidate_order':'randomized per source entry',
            'limitations':'Reviewer knew earlier high-level findings, not a preregistered independent audit. Style and one API error string can reveal origin. Original paid judge ratings were not used to assign these scores.'},
        conditions={'source_entries':500,'translations':2000,'ocr':False,'frame_generation':False,'enhancer':False,
            'hardware':'Intel i5-13420H / RTX 4050 Laptop 6 GiB / Windows 11',
            'timing':'Reused original measurements. Local models serial, API overlapped local runs. Warmup separate. No new translation calls.'},
        models=summaries, cases=combined)
    (data/'review.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),'utf-8')
    for file in [a.review/'direct-review-ratings.txt',a.review/'review-mapping.json',a.review/'review-blind-input.jsonl',*sorted(a.review.glob('direct-review-notes*.json'))]:
        shutil.copy2(file,data/file.name)
    with (data/'translations-scored.csv').open('w',encoding='utf-8-sig',newline='') as stream:
        w=csv.writer(stream);w.writerow(['id','English','reference_cs','model','translation','meaning_0_4','fluency_0_4','score_100','latency_ms','note_cs','error'])
        for c in combined:
            for m,v in c['models'].items():w.writerow([c['id'],c['en'],c['reference_cs'],m,v['translation'],v['meaning'],v['fluency'],v['score'],v['ms'],v['note_cs'],v['error']])
    provenance={'inputs':{f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in [a.benchmark/'dataset500.jsonl',*[a.benchmark/(m+'.jsonl') for m in LABELS],a.review/'direct-review-ratings.txt']},'reviewed_entries':500,'reviewed_outputs':2000,'notes_cases':len(notes)}
    (data/'provenance.json').write_text(json.dumps(provenance,indent=2),'utf-8')
    shutil.copy2(Path(__file__),data/'render_direct_review.py')
    esc=html.escape
    table=''.join(f'<tr><th>{esc(m["label"])}</th><td><strong>{m["score"]:.1f}</strong></td><td>{m["meaning"]:.2f}</td><td>{m["fluency"]:.2f}</td><td>{m["usable"]}/500</td><td>{m["material"]}/500</td><td>{m["median_ms"]/1000:.2f} s</td><td>{m["p95_ms"]/1000:.2f} s</td></tr>' for m in summaries)
    highlights=[('w500-470','Vodopád, nebo šedý zákal?'),('w500-343','Plynulý text v nesprávném jazyce'),('w500-215','Ztracená negace'),('w500-393','Zvládnu to, nebo nezvládnu?'),('w500-032','Chybuje i API'),('w500-161','Chybná API odpověď zůstává ve skóre'),('w500-367','Obtížná replika pro všechny čtyři'),('w500-437','Ani lidská reference není neomylná')]
    cards=''
    for ident,title in highlights:
        c=next(c for c in combined if c['id']==ident)
        candidates=''.join(f'<div class="candidate"><h4>{esc(LABELS[m])} <span class="score">{v["score"]:g}/100</span></h4><p>{esc(v["translation"] or "[Překlad chybí]")}</p><p class="meta">{esc(v["note_cs"])}</p></div>' for m,v in c['models'].items())
        cards+=f'<details><summary>{esc(title)} · {ident}</summary><blockquote lang="en">{esc(c["en"])}</blockquote><p class="meta">{esc(c["note_cs"])}</p><div class="candidates">{candidates}</div><a href="#case-{ident}">Otevřít ve všech výsledcích →</a></details>'
    bestlocal=next(m for m in summaries if m['model']!='gpt-6-luna')
    page=f'''<!doctype html><html lang="cs"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FrameLab — vlastní rozbor 2 000 překladů a pořadí modelů</title><link rel="stylesheet" href="../../guide/assets/remote-subtitles.css"><link rel="stylesheet" href="review.css"></head><body><main>
<nav><a href="../../?lang=cs">FRAME/LAB</a><a href="../../#download">Stáhnout FrameLab</a><a href="../2026-11-23-api500/">Původní měření a cenový účet</a><a href="#english">English summary</a></nav>
<p class="tag">500 REPLIK · 4 MODELY · 2 000 POSOUZENÝCH PŘEKLADŮ</p><h1>Který překladač<br>rozumí tomu, co říká?</h1>
<p class="lead">Vlastní rozbor uložených překladů asistentem Codex. Každý výstup byl přečten a jednotlivě ohodnocen podle anglického zadání. Nové hodnocení použilo <strong>0 dalších placených API požadavků</strong>.</p>
<p class="notice">Nejde o lidský audit ani objektivní procento přesnosti. Jde o úsudek jednoho AI asistenta, který pořizuje tuto dokumentaci. Značky modelů byly při bodování nahrazené A–D a pořadí promíchané u každé repliky. Styl mohl model prozradit; známé dřívější souhrny nelze „odmyslet“. Předchozí placené hodnocení zůstává odděleně v archivu.</p>
<h2>Výsledek na této sadě</h2><div class="table"><table><thead><tr><th>Model</th><th>Skóre / 100</th><th>Význam / 4</th><th>Čeština / 4</th><th>Použitelné*</th><th>Významová chyba*</th><th>Medián překladu</th><th>P95</th></tr></thead><tbody>{table}</tbody></table></div>
<p>Nejvyšší výsledné skóre: <strong>{esc(summaries[0]['label'])} ({summaries[0]['score']:.1f}/100)</strong>. Nejlepší místní model v této sadě: <strong>{esc(bestlocal['label'])} ({bestlocal['score']:.1f}/100)</strong>. Nejnižší skóre: <strong>{esc(summaries[-1]['label'])} ({summaries[-1]['score']:.1f}/100)</strong>. Výběr podle rychlosti se může lišit od výběru podle češtiny; časy ve stejné tabulce ukazují tento kompromis.</p>
<p>* Použitelné: význam i čeština alespoň 3/4. Významová chyba: význam nejvýše 2/4. Ani „použitelné“ neznamená bezchybné. Skóre počítá každou repliku stejně a vzniká jako <code>100 × (3 × význam + čeština) / 16</code>. Význam má váhu 75 %, čeština 25 %. Chybějící výstup nebo text v jiném jazyce dostává 0 celkem, i když v jiném jazyce význam zachoval.</p>
<h2>Největší rozdíly, včetně slabých míst vítěze</h2><p>Tyto příklady vysvětlují chyby, nikoli jejich četnost. Úplná tabulka níže umožňuje seřadit všechny repliky podle největšího rozdílu nebo od nejhoršího výstupu zvoleného modelu.</p>{cards}
<h2>Co stojí test a co běžný překlad</h2><p>Původních 500 překladových požadavků: doloženě <strong>$0.0186901</strong>. Předchozí samostatné API hodnocení: <strong>$0.70905</strong>. Celá tehdejší zkouška včetně filtru, diagnostiky a integračních volání: doloženě <strong>$0.7306883</strong>, plus jedna odpověď bez zachované spotřeby. Toto nové hodnocení uložených výsledků: <strong>$0 nových volání uživatelského API</strong>.</p><p>Částky jsou odhady z tokenů, nikoli faktura nebo aktuální zůstatek. Placený posuzovatel se při normálním používání FrameLabu nespouští. <a href="../2026-11-23-api500/data/cost-ledger.json">Úplný původní účet</a>.</p>
<h2>Jak číst skóre a jeho omezení</h2><ul><li><strong>Význam:</strong> 4 = zachován; 3 = menší ztráta nebo posun; 2 = podstatná chyba; 1 = převážně nesprávně; 0 = chybí nebo nesouvisí.</li><li><strong>Čeština:</strong> 4 = přirozená; 3 = drobné neobratnosti; 2 = více gramatických problémů; 1 = těžko srozumitelná; 0 = chybí nebo není česká.</li><li>Rozhoduje anglický originál. Česká reference je pomůcka; přijatelné synonymum nebo rod, který angličtina neurčuje, není sám o sobě chyba. Případy 042 a 437 dokládají chyby reference.</li><li>500 různých položek z fantasy hry Battle for Wesnoth, ne 500 nezávislých žánrů. Některé repliky jsou si podobné; modely mohly veřejná data vidět při trénování. Výsledky neprokazují kvalitu ostatních jazyků nebo OCR.</li><li>Časy pocházejí z původního textového testu na i5-13420H / RTX 4050 Laptop 6 GiB. <strong>OCR, FG a enhancer byly vypnuté.</strong> Lokální modely běžely postupně, API souběžně s lokálním testem. Nejde o latenci titulku ve hře.</li><li>Starší testy Hy-MT Q8 a dalších kvantizací používaly jinou sadu 300 položek. Do tohoto pořadí se nepřimíchávají. Srovnání CUDA/Vulkan měří jinou, malou sadu a výkon; nemění zde hodnocené výstupy.</li><li>API případ 161 selhal. Pozdější úspěšný diagnostický pokus jej nenahrazuje. U každého skóre lze dohledat skutečný původní výstup.</li></ul>
<h2 id="english">English summary</h2><p>All 2,000 saved EN→CS outputs were individually read and scored by the Codex assistant without further paid API requests. This is a single AI review, not independent human evaluation. Candidate identities were shuffled per entry. The score weights meaning 75% and Czech fluency 25%; absent or non-Czech output scores zero. This fantasy dataset and single hardware run do not establish universal accuracy. Original translation timing, failure and costs remain unchanged. See the downloadable JSON/CSV for all ratings, original text and Czech explanations.</p>
<h2 id="all">Všech 500 replik vedle sebe</h2><div class="controls"><label>Hledat<input id="search" type="search" placeholder="ID, anglický nebo český text"></label><label>Řazení<select id="sort"><option value="spread">Největší rozdíl modelů</option><option value="id">Původní pořadí</option><option value="worst">Nejhorší zvolený model</option><option value="best">Nejlepší zvolený model</option></select></label><label>Model pro řazení<select id="model">{''.join(f'<option value="{m}">{esc(n)}</option>' for m,n in LABELS.items())}</select></label><label><input id="notes" type="checkbox">Jen s vysvětlením</label></div><p id="count" role="status">Načítám uložené výsledky…</p><div class="controls"><button id="prev">← Předchozí</button><button id="next">Další →</button></div><div id="cases"></div>
<h2>Data ke kontrole</h2><p><a href="data/translations-scored.csv">2 000 překladů a skóre — CSV</a> · <a href="data/review.json">Úplná data JSON</a> · <a href="data/direct-review-ratings.txt">Původní anonymní hodnocení</a> · <a href="data/review-mapping.json">Odhalení kandidátů A–D</a> · <a href="data/provenance.json">SHA-256 vstupů</a> · <a href="data/SHA256SUMS.txt">Kontrolní součty</a> · <a href="../2026-11-23-api500/data/dataset-manifest.json">Dataset a zdroj</a> · <a href="../2026-11-23-api500/data/COPYING-wesnoth.txt">GPL a původní licence datasetu</a></p><p class="meta">Texty hry a české reference: autoři Battle for Wesnoth a české lokalizace, GPL-2.0-or-later, přesný zdroj v manifestu. Konkrétní poznámky jsou jen u označených případů; prázdná poznámka neznamená, že výstup nebyl posouzen.</p></main><script src="review.js"></script></body></html>'''
    (out/'index.html').write_text(page,'utf-8')
    for name in ('review.css','review.js'):shutil.copy2(ROOT/'tools/direct_review'/name,out/name)
    (data/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.name+'\n' for f in sorted(data.iterdir()) if f.name!='SHA256SUMS.txt'),'utf-8')
    print(json.dumps(summaries,ensure_ascii=False,indent=2))

if __name__ == '__main__':main()
