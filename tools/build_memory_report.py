"""Assemble the reviewed memory experiment. Does not run or judge any model."""
from pathlib import Path
import argparse,csv,hashlib,html,json,math,shutil,statistics,zipfile

ROOT=Path(__file__).resolve().parents[1]
MODES=['baseline','naive','guarded','timeline']
RUNS={'cuni-q3km':['heldout-cuni','timeline-cuni'], 'gpt-6-luna':['heldout-luna','timeline-luna'],
      'translategemma-12b-q4km':['heldout-gemma12'], 'server-gemma4-26b-a4b-q5ks':['server-gemma4']}
LABELS={'cuni-q3km':'CUNI-MH 7B Q3_K_M · RTX 4050','gpt-6-luna':'GPT-6 Luna · OpenAI API',
        'translategemma-12b-q4km':'TranslateGemma 12B Q4_K_M · RTX 4050 + RAM',
        'server-gemma4-26b-a4b-q5ks':'Gemma 4 26B A4B Q5_K_S · external RTX 3090'}

def stats(values):
    a=sorted(values)
    return dict(n=len(a),median=statistics.median(a),p95=a[math.ceil(len(a)*.95)-1],mean=statistics.mean(a)) if a else {}

def t(cs,en,tag='span',cls=''):
    esc=lambda x:html.escape(str(x),quote=True)
    return f'<{tag} data-cs="{esc(cs)}" data-en="{esc(en)}"'+(f' class="{cls}"' if cls else '')+f'>{esc(en)}</{tag}>'

def main():
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('--destination',type=Path,default=ROOT/'website/runs/2026-11-24-context-memory');a=p.parse_args()
    source=a.source.resolve();dest=a.destination;data=dest/'data';data.mkdir(parents=True,exist_ok=True)
    dataset=json.loads((source/'dataset-timeline-v1.json').read_text(encoding='utf-8'))
    cases=[r for r in dataset['rows'] if r['split']=='heldout'];assert len(cases)==88
    reviews={}
    for path in source.glob('review-*.json'):
        for r in json.loads(path.read_text(encoding='utf-8')):
            key=(r['model'],r['id'],r['mode'])
            if key in reviews:raise ValueError('Duplicate assessment '+str(key))
            reviews[key]=r
    records=[];metadata={};summary=[]
    for model,folders in RUNS.items():
        entries=[];meta=[]
        for folder in folders:
            run=source/folder;m=json.loads((run/'metadata.json').read_text(encoding='utf-8'))
            if not m.get('completed_utc') or m['completed']!=m['planned']:raise ValueError('Incomplete run '+folder)
            meta.append(m)
            rows=[json.loads(x) for x in (run/'outputs.jsonl').read_text(encoding='utf-8').splitlines()]
            for row in rows:
                key=(model,row['id'],row['mode'])
                if key not in reviews:raise ValueError('Unreviewed output '+str(key))
                row=dict(row,review=reviews[key]);entries.append(row)
            for name in ('metadata.json','outputs.jsonl'):
                (data/folder).mkdir(exist_ok=True);shutil.copy2(run/name,data/folder/name)
        assert len(entries)==352 and len({(x['id'],x['mode']) for x in entries})==352
        metadata[model]=meta
        for mode in MODES:
            group=[x for x in entries if x['mode']==mode];scored=[x['review']['score'] for x in group if x['review'].get('score') is not None]
            gender=[x['review']['feature_pass'] for x in group if x['review'].get('feature')=='known_speaker_gender']
            lexical=[x['review']['feature_pass'] for x in group if x['review'].get('feature')=='name_or_word_sense']
            summary.append(dict(model=model,mode=mode,scored_n=len(scored),score=25*statistics.mean(scored),
                                major=sum(v<=1 for v in scored),gender_pass=sum(gender),gender_n=len(gender),
                                lexical_pass=sum(lexical),lexical_n=len(lexical),latency_ms=stats([x['ms'] for x in group]),
                                errors=sum(bool(x.get('error')) for x in group),
                                server_compute_ms=stats([x['metrics']['timings']['prompt_ms']+x['metrics']['timings']['predicted_ms'] for x in group if x.get('metrics',{}).get('timings')]),
                                input_tokens=stats([x['usage'].get('prompt_tokens',x['usage'].get('input_tokens',0)) for x in group if x.get('usage')]) ))
        records.extend({k:v for k,v in row.items() if k not in ('prompt','context')} for row in entries)
    assert len(records)==1408
    api_cost=0;pilots=[]
    for path in source.glob('*/metadata.json'):
        m=json.loads(path.read_text(encoding='utf-8'))
        if path.parent.name in ('pilot-luna','heldout-luna','timeline-luna'):api_cost+=m.get('api_cost_usd',0)
        if path.parent.name.startswith('pilot-'):
            pilots.append(dict(name=path.parent.name,completed=m.get('completed'),planned=m.get('planned')))
            target=data/path.parent.name;target.mkdir(exist_ok=True)
            for name in ('metadata.json','outputs.jsonl'):shutil.copy2(path.parent/name,target/name)
    for name in ['dataset-v1.json','dataset-timeline-v1.json','review-cuni.json','review-luna.json','review-gemma12.json','review-server.json','translation_memory-v1.py','translation_memory-v2.py','server-environment.json']:
        shutil.copy2(source/name,data/name)
    extra=data/'profile-only-cuni';extra.mkdir(exist_ok=True);shutil.copy2(source/'profile-only-cuni/outputs.jsonl',extra/'outputs.jsonl')
    frozen=data/'frozen-tools';frozen.mkdir(exist_ok=True)
    for path in (source/'frozen-tools').iterdir():
        if path.is_file():shutil.copy2(path,frozen/path.name)
    (frozen/'README.txt').write_text('Frozen benchmark code used on 2026-11-24. Keep JSON catalogs beside these scripts. Prompts and source snapshots also appear in the raw outputs. Application code may evolve separately. / Zmrazeny kod testu; katalogy JSON ponechte vedle skriptu. Presne prompty a zdrojova historie jsou i v surovych vystupech. Aplikacni kod se muze vyvijet samostatne.\n',encoding='utf-8')
    report=dict(version=1,models=LABELS,modes=MODES,cases=cases,records=records,summary=summary,metadata=metadata,
                api_translation_cost_usd=api_cost,paid_judge_calls=0,pilots=pilots,main_outputs=len(records),
                provenance='Owner-provided public debug log. 32,844 OCR completions + 15,190 style lines + 9 resets replayed causally. 64 target jobs and 24 authored stress cases. No verified game speaker labels.',
                review='Codex direct, unblinded assessment in the task conversation. No paid or local judge model. 59 of 64 real jobs scored; 5 corrupted/non-English jobs retained but not scored. Synthetic features separate. Not human-validated accuracy.')
    (data/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    with (data/'translations.csv').open('w',encoding='utf-8-sig',newline='') as f:
        fields=['model','id','mode','source','translation','ms','score','feature','feature_pass','note_en','note_cs','error'];w=csv.DictWriter(f,fieldnames=fields);w.writeheader()
        for r in records:
            row={k:r.get(k,r['review'].get(k,'')) for k in fields}
            for k,v in row.items():
                if isinstance(v,str) and v[:1] in ('=','+','-','@'):row[k]="'"+v
            w.writerow(row)
    (data/'sha256.json').write_text(json.dumps({p.relative_to(data).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(data.rglob('*')) if p.is_file() and p.name!='sha256.json'},indent=2),encoding='utf-8')
    with zipfile.ZipFile(dest/'evidence.zip','w',zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(data.rglob('*')):
            if path.is_file():archive.write(path,path.relative_to(dest))
    body='<header class="page-intro"><span class="eyebrow">CONTEXT LAB · 24 / 11 / 2026</span>'+t('Pomůže překladači paměť rozhovoru?','Can conversation memory help translation?','h1')+t('Čtyři modely, stejné titulky a skutečný OCR balast mezi nimi. Porovnáváme samostatný titulek, syrovou historii, omezenou paměť a přepis s časy.','Four models, the same subtitles and real OCR clutter between them. Compare a standalone subtitle, raw history, bounded memory and a timestamped transcript.','p','lead')+'</header>'
    body+='<p><a href="live.html">'+t('Nově: ověření paměti přímo v běžícím FrameLabu','New: memory tested in the actual running FrameLab')+'</a> · <a href="speed50.html">'+t('Server versus OpenAI: 50 + 50 měření','Server versus OpenAI: 50 + 50 measurements')+'</a></p>'
    body+='<nav class="jump-links">'+''.join(f'<a href="#{i}">{t(cs,en)}</a>' for i,cs,en in [('findings','Zjištění','Findings'),('scores','Skóre a odezva','Scores & latency'),('examples','Skutečné výstupy','Actual outputs'),('method','Metoda a sestavy','Method & hardware'),('downloads','Data','Data')])+'</nav>'
    body+='<div class="notice warning">'+t('Výzkumný test EN → CS. Skóre je přímé, neslepé posouzení Codexem, nikoli měření přesnosti člověkem. 1 408 výstupů pochází z 88 cílových případů; neznamená to 1 408 nezávislých rozhovorů.','An EN → CS diagnostic. Scores are a direct, unblinded Codex assessment, not human-validated accuracy. The 1,408 outputs come from 88 target cases; they are not 1,408 independent conversations.')+'</div>'
    body+='<div class="cards"><article class="card"><div class="number">1 408</div>'+t('porovnaných výstupů','compared outputs','p')+'</article><article class="card"><div class="number">64 + 24</div>'+t('herních OCR úloh + zátěžových scén','game OCR jobs + authored stress cases','p')+'</article><article class="card"><div class="number">$'+f'{api_cost:.5f}'+'</div>'+t('Luna: překlady včetně pilotu. Hodnoticí API: $0.','Luna: translations including the pilot. Judge API: $0.','p')+'</article></div>'
    body+='<section class="section" id="findings">'+t('Paměť potřebuje pravidla','Memory needs rules','h2')+'<div class="cards">'
    findings=[('Kontext umí opravit význam','Context can fix meaning','U Luny se „license“ v debatě o létání změnilo z řidičáku na pilotní průkaz. Časovaná varianta také správně přeložila „Are you mad?“ jako „Zlobíš se?“.','With Luna, “license” in a flying conversation changes from a driving licence to a pilot licence. The timestamped variant also correctly renders “Are you mad?” as asking about anger.'),
              ('CUNI může opakovat staré věty','CUNI can repeat old lines','V tomto testu širší historie zhoršila CUNI. Časovaný přepis část chyb omezil, ale nepřekonal jeho překlad bez paměti. Samotný větší prompt není oprava.','In this test, broader history made CUNI worse. The timestamped transcript reduced some failures but did not beat its standalone translation. A bigger prompt alone is not a fix.'),
              ('Známý rod není rozpoznaná postava','Known gender is not speaker recognition','U 12 řízených příkladů dostal model profil postavy předem. Zlepšení rodu dokazuje využití této informace, nikoli automatické poznání mluvčího v reálné hře. V herním exportu spolehlivá označení chybějí.','In 12 controlled examples, the character profile was supplied in advance. Better gender agreement shows use of this information, not automatic recognition of real game speakers. The gameplay export lacks reliable labels.'),
              ('Velká Gemma na jiném počítači','A large Gemma on another computer','Externí Gemma 4 běží na serverové RTX 3090, s kontextem 214 272 tokenů a při kontrole přibližně 22 GB obsazené VRAM. Do testu dostává krátkou historii. Jde o samostatnou volitelnou sestavu.','The external Gemma 4 runs on a server RTX 3090, with a 214,272-token context and roughly 22 GB VRAM occupied at inspection. The test sends a short history. This is a separate optional setup.')]
    for cs,en,desc_cs,desc_en in findings:body+='<article class="card">'+t(cs,en,'h3')+t(desc_cs,desc_en,'p')+'</article>'
    body+='</div></section><section class="section" id="scores">'+t('Stejné vstupy, čtyři způsoby práce s pamětí','Same inputs, four memory strategies','h2')+t('Skóre je na 59 hodnotitelných herních úlohách. Rod je zvláštní zkouška 12 označených scén. Odezva je celý požadavek bez živého OCR, FG a enhanceru.','Scores cover 59 assessable game jobs. Gender is a separate test of 12 labelled scenes. Latency is the full request, with live OCR, FG and enhancement off.','p')+'<div id="memory-scores"></div></section>'
    body+='<section class="section" id="examples">'+t('Prohlédni si konkrétní repliku','Inspect an individual line','h2')+'<div class="filters"><label>'+t('Model','Model')+'<select id="memory-model"></select></label><label>'+t('Případ','Case')+'<select id="memory-case"></select></label></div><div class="actions"><button id="memory-prev">←</button><button id="memory-next">→</button></div><div id="memory-example" aria-live="polite"></div></section>'
    body+='<section class="section" id="method">'+t('Co přesně se testovalo','What exactly was tested','h2')+'<div class="method-grid">'
    methods=[('Surové pořadí a filler','Raw sequence and filler','Paměť se naplnila časově předcházejícími OCR záznamy, včetně SPZ, nápisů, krátkých odpovědí a chybných fragmentů. Nevybírali jsme jen čistý příběh. Staré české překlady se do paměti nedostaly.','Memory was populated from earlier OCR events, including plates, signs, short replies and faulty fragments. This is not a clean-story-only test. Old Czech translations never entered memory.'),
             ('Výběr a pilot','Selection and pilot','64 úloh v 16 úsecích po čtyřech, rozmístěných v původní relaci, plus 24 vytvořených scén. Oddělený pilot měl šest případů. CUNI mělo dvě pilotní instrukce; přísnější zákaz přidávání textu problém neodstranil.','64 jobs in 16 four-job windows spread through the original session, plus 24 authored scenes. The separate pilot had six cases. CUNI had two pilot instructions; a stricter ban on adding text did not solve the problem.'),
             ('Časovaný přepis','Timestamped transcript','Navazující průzkum na stejných cílových větách: spojuje sousední prodlužované OCR titulky, zachovává opakování po jiné replice, přidává začátek/konec a kandidátní úsek po pauze. Posílá nejvýše šest replik z posledních 60 sekund. Časová blízkost nepotvrzuje stejného mluvčího ani téma.','An exploratory follow-up on the same target lines: stitches adjacent growing OCR captions, keeps repetition after another turn, adds start/end times and a candidate segment after a gap. Sends up to six turns from the last 60 seconds. Temporal proximity does not prove a shared speaker or topic.'),
             ('Co porovnání neizoluje','What the comparison does not isolate','Časovaná varianta mění i spojování a výběr textu, nejen čísla času. CUNI a Luna ji absolvovaly po hlavním běhu; obě Gemmy ji mají mezi promíchanými variantami. U syntetiky se liší dostupnost ověřeného profilu. Z výsledků nelze určit čistý samostatný účinek časových značek.','The timestamped variant also changes stitching and selection, not just time numbers. CUNI and Luna ran it after the main run; both Gemmas interleave it with other arms. Synthetic cases also differ in supplied verified profiles. These results do not isolate the effect of timestamps alone.'),
             ('Notebook a server','Laptop and server','Notebook: Intel i5-13420H, 32 GB RAM, RTX 4050 Laptop 6 GB, Windows 11. CUNI celé na GPU; TranslateGemma 12B používá GPU i RAM. Server: Ubuntu, RTX 3090 24 GB, Gemma 4 26B A4B Heretic Q5_K_S přes llama.cpp. Jde o jiný model než TranslateGemma.','Laptop: Intel i5-13420H, 32 GB RAM, RTX 4050 Laptop 6 GB, Windows 11. CUNI fully on GPU; TranslateGemma 12B uses GPU and RAM. Server: Ubuntu, RTX 3090 24 GB, Gemma 4 26B A4B Heretic Q5_K_S via llama.cpp. It is a different model from TranslateGemma.'),
             ('Odezva a cena','Latency and cost','Jeden lokální model najednou; zahřátí mimo měření. Internetové stahování mohlo běžet současně. U API se může měnit odezva i odpověď při teplotě 0. Náklady počítáme z vrácených tokenů Luny: $0,10 vstup / $0,01 cache / $0,50 výstup za milion tokenů; žádné placené hodnocení.','One local model at a time; warm-up excluded. An Internet download could run concurrently. API latency and outputs can vary even at temperature zero. Luna cost is calculated from returned tokens: $0.10 input / $0.01 cached / $0.50 output per million tokens; no paid assessment.'),
             ('Rubrika','Rubric','0–4 body za zachování významu a čitelnost: 4 bez podstatné chyby, 3 malý posun či neobratnost, 2 důležitá změna nebo přidaný obsah, 1 ztracený hlavní smysl či dlouhé opakování, 0 jiná/chybějící replika. Skóre /100 = 25 × průměr. Rod neznámých mluvčích nehodnotíme. Pět nečitelných nebo již českých vstupů je mimo skóre, výstupy zůstávají zveřejněné.','0–4 for meaning preservation and readability: 4 no material error, 3 minor shift/awkwardness, 2 important change or added content, 1 main meaning lost or long repetition, 0 different/missing line. Score /100 = 25 × mean. Unknown speaker gender is not graded. Five corrupted/already-Czech inputs are excluded from scoring, with all outputs retained.')]
    for cs,en,dc,de in methods:body+='<article>'+t(cs,en,'h3')+t(dc,de,'p')+'</article>'
    body+='</div><p><a href="https://aclanthology.org/2024.wmt-1.16.pdf">CUNI paper, Appendix A</a> · <a href="https://developers.openai.com/api/docs/models/gpt-6-luna">GPT-6 Luna pricing</a></p></section>'
    body+='<section class="section" id="downloads">'+t('Výsledky lze zkontrolovat a zopakovat','Audit and reproduce the results','h2')+'<div class="download-links">'
    for path,cs,en in [('evidence.zip','Celý test: překlady, prompty, posouzení a zmrazený kód · ZIP','Full test: outputs, prompts, assessments and frozen code · ZIP'),('data/report.json','Kompletní výsledky a posouzení · JSON','Complete results and assessments · JSON'),('data/translations.csv','Všechny překlady · CSV','All translations · CSV'),('data/dataset-timeline-v1.json','Vstupy a přesná historie','Inputs and exact history'),('data/sha256.json','Kontrolní součty','Checksums'),('../2026-11-20-3h/','Původní tříhodinový záznam','Original three-hour log'),('../../results/','Všechna měření','All measurements')]:body+=f'<a href="{path}">{t(cs,en)}</a>'
    body+='</div><p><a href="https://github.com/tomik520i/FrameLab/blob/main/tools/benchmark_translation_memory.py">benchmark_translation_memory.py</a> · <a href="https://github.com/tomik520i/FrameLab/blob/main/docs/CONVERSATION-MEMORY.md">Reproduction / návrh a opakování testu</a></p></section><footer class="footer">FRAME/LAB · CONTEXT LAB</footer>'
    head='<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">'+t('FrameLab — paměť překladače','FrameLab — translation memory','title')
    for path in ['assets/fonts/fonts.css','explore.css','site-i18n.css','site-navigation.css','site-images.css']:head+=f'<link rel="stylesheet" href="../../{path}">'
    head+='<link rel="stylesheet" href="memory.css">'
    for path in ['site-i18n.js','site-navigation.js','site-images.js']:head+=f'<script defer src="../../{path}"></script>'
    head+='<script defer src="memory.js"></script>'
    (dest/'index.html').write_text('<!doctype html><html lang="en" data-site-page="runs/2026-11-24-context-memory/index.html"><head>'+head+'</head><body><main id="main">'+body+'</main></body></html>',encoding='utf-8')
    print('Reviewed memory report assembled:',dest)

if __name__=='__main__':main()
