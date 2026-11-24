"""Report actual OCR/translation/display evidence; no model calls or auto judge."""
from pathlib import Path
import argparse,json,html,hashlib,shutil,statistics,zipfile
from build_memory_report import t,stats

# Direct assessment after inspecting the actual rendered outputs. Only the
# repeated ambiguous target is graded, not overall model quality. None=not shown.
REVIEW={
 'openai-bar-off':(['Dostaň odtud tu děvku.']*3,[4,4,1]),
 'openai-bar-selected':(['Vyhoďte tu couru odsud.',None,'Dostaňte tu motyku pryč.'],[4,0,4]),
 'openai-bar-selected-async-v2':(['Vyhoďte tu děvku ven.','Dostaňte tu couru odsud.','Vypadněte s tou motykou odsud.'],[4,4,3]),
 'openai-garden-off-async-v2':(['Dostaň tu couru pryč.']*3,[1,1,4]),
 'openai-garden-selected-async-v2':(['Dostaň tu motyku pryč.','Vyneste odsud tu motyku.','Dostaňte tu couru odsud.'],[4,4,4]),
 'openai-bar-recent-async-v2':(['Dostaňte tu couru odsud.','Dostaň tu motyku odtud.','Vyneste odtud tu motyku.'],[4,1,4]),
 'openai-garden-recent-async-v2':(['Dostaň tu motyku pryč.','Dostaňte odtud tu motyku.','Odveď tu couru odsud.'],[4,4,4]),
 'custom-bar-off-async-v2':(['Dostaň tu děvku pryč.']*3,[4,4,1]),
 'custom-bar-selected-async-v2':(['Dostaň tu mrchu pryč.']*3,[3,3,1]),
 'custom-garden-selected-async-v2':(['Dostaň tu motykou pryč.','Dostaň tu motykou pryč.','Dostaň tu mrchu pryč.'],[3,3,3]),
 'custom-garden-off-async-v2':(['Dostaň tu děvku pryč.']*3,[1,1,4]),
 'custom-bar-selected-async-v3':(['Dostaň tu mrdu pryč.','Dostaň tu mrchu pryč.','Dostaň tu motyku pryč.'],[2,3,4]),
 'openai-bar-selected-async-v3':(['Dostaňte tu couru odtud.','Dostaňte tu couru odsud.','Vypadněte s tou motykou odsud.'],[4,4,3]),
 'custom-garden-selected-async-v3':(['Dostaň tu motyku pryč.','Dostaň tu motykou pryč.','Dostaň tu mrchu pryč.'],[4,3,3]),
 'openai-garden-selected-async-v3':(['Odnes tu motyku pryč.','Vyneste tu motyku ven.','Dostaňte tu couru odsud.'],[4,4,4]),
 'custom-bar-selected-async-v4':(['Dostaň tu mrdu pryč.','Dostaň tu mrchu pryč.','Dostaň tu mrchu pryč.'],[2,3,1]),
 'openai-bar-selected-async-v4':(['Vyhoďte tu couru odtud.','Vypadněte s tou courou odsud.','Odnes tu motyku pryč.'],[4,3,4]),
 'custom-garden-selected-async-v4':(['Dostaň tu motyku pryč.','Dostaň tu motykou pryč.','Dostaň tu mrchu pryč.'],[4,3,3]),
 'openai-garden-selected-async-v4':(['Dostaňte tu motyku pryč.','Odnes tu motyku.','Dostaňte tu couru odsud.'],[4,4,4]),
}

def read_lines(path):return [json.loads(x) for x in path.read_text(encoding='utf-8').splitlines() if x.strip()]
def main():
 p=argparse.ArgumentParser();p.add_argument('source',type=Path);a=p.parse_args();source=a.source.resolve()
 dest=Path(__file__).resolve().parents[1]/'website/runs/2026-11-24-context-memory';data=dest/'data/live';data.mkdir(parents=True,exist_ok=True)
 runs=[]
 for name,(expected,grades) in REVIEW.items():
  folder=source/'live-runs'/name;meta=json.loads((folder/'metadata.json').read_text(encoding='utf-8'));assert meta['exit_code']==0 and meta['completed_utc']
  events=read_lines(folder/'events.jsonl');shown=[x for x in events if x['event']=='caption_display'];by_id={x['job_id']:x for x in shown}
  targets=[x for x in events if x['event']=='ocr_complete' and x.get('translate')==1 and x.get('source')=='Get that hoe out of here.'];assert len(targets)==3,(name,len(targets))
  worker_rows=read_lines(folder/'worker.jsonl');worker_by_id={x.get('event_id'):x for x in worker_rows}
  actual=[by_id.get(x['job_id'],{}).get('translation') for x in targets];assert actual==expected,(name,actual,expected)
  reviewed=[dict(phase=i,job_id=r['job_id'],output=actual[i],score=grades[i],display_ms=by_id.get(r['job_id'],{}).get('capture_to_display_ms'),output_sha256=hashlib.sha256((actual[i] or '').encode()).hexdigest(),memory_status=worker_by_id.get(str(r['job_id']),{}).get('status','')) for i,r in enumerate(targets)]
  metrics=json.loads((folder/'summary.json').read_text(encoding='utf-8'))['metrics'];cost=None;usage=[]
  if (folder/'api-usage.jsonl').exists():
   usage=read_lines(folder/'api-usage.jsonl');cost=0.
   for row in usage:
    u=row['usage'];d=u.get('input_tokens_details',{});c=d.get('cached_tokens',0);w=d.get('cache_creation_tokens',d.get('cache_write_tokens',0));cost+=(max(0,u['input_tokens']-c-w)*.10+c*.01+w*.125+u['output_tokens']*.50)/1e6
  run=dict(id=name,provider=meta['provider'],scene=meta['scene'],mode=meta['mode'],synchronous=name=='openai-bar-selected',rendered_count=len(shown),display_latency_ms=stats([x['capture_to_display_ms'] for x in shown]),target_review=reviewed,metrics=metrics,cost_usd=cost,completed_api_calls=len(usage),displayed=shown,drops=[x for x in events if x['event'] in ['translation_dropped','translation_expired','caption_expired']],worker=worker_rows,memory_errors=[dict(event_id=x.get('event_id'),error=x['memory']['error']) for x in worker_rows if x.get('memory',{}).get('error')],variant=('v1' if name=='openai-bar-selected' else name.rsplit('-',1)[-1] if '-async-v' in name else 'v2'))
  runs.append(run);(data/name).mkdir(exist_ok=True)
  for f in folder.iterdir():
   if f.suffix in ['.json','.jsonl','.csv','.png','.jpg']:shutil.copy2(f,data/name/f.name)
 report=dict(scope=f'Live native-app pilot: two authored scenes, {len(runs)} runs. Not the requested future 25 Czech + 50 multilingual showcase. No paid judge. Target scores are direct unblinded Codex review of {sum(len(r["target_review"]) for r in runs)} render opportunities, not full-dialogue quality or population accuracy.',runs=runs)
 (data/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 for name in ['bar.json','garden.json','bar.ass','garden.ass']:shutil.copy2(source/'live-scenes'/name,data/name)
 selection=dest/'data/selection-review';selection.mkdir(exist_ok=True)
 for name in ['context-validation-v1.json','context-confirmation-v1.json','validation-summary.json','confirmation-summary.json','blind-validation.json','blind-validation-key.json','blind-validation-review.json','blind-confirmation.json','blind-confirmation-key.json','blind-confirmation-review.json']:
  shutil.copy2(source/name,selection/name)
 for stage in ['validation','confirmation']:
  shutil.copy2(source/'context-assessments'/f'{stage}.json',selection/f'{stage}-assessments.json')
  for provider in ['luna','server']:
   name=f'context-{stage}-{provider}';(selection/name).mkdir(exist_ok=True)
   for file in ['metadata.json','outputs.jsonl']:shutil.copy2(source/name/file,selection/name/file)
 for frozen_name in ['context-building-frozen-v2','context-building-frozen-v3']:
  target=selection/frozen_name;target.mkdir(exist_ok=True)
  for file in (source/frozen_name).iterdir():
   if file.is_file() and file.suffix in ['.py','.json','.txt']:shutil.copy2(file,target/file.name)
 body='<header class="page-intro"><span class="eyebrow">FRAME/LAB · LIVE CONTEXT PILOT</span>'+t('Co paměť udělá ve skutečném programu?','What does memory do in the actual app?','h1')+t(f'Obraz → OCR → filtr → vybraný model a jeho vlastní paměť → zobrazený titulek. Dvě scény, {len(runs)} skutečných běhů.',f'Picture → OCR → filter → selected model and its own memory → displayed caption. Two scenes, {len(runs)} real runs.','p','lead')+'</header>'
 body+='<nav class="jump-links"><a href="#outputs">'+t('Skutečné výstupy','Actual outputs')+'</a><a href="#all-runs">'+t('Použitý kontext','Actual context')+'</a><a href="#method">'+t('Metoda a meze','Method and limits')+'</a></nav>'
 body+='<section class="section">'+t('Výsledek i selhání','Results and failures','h2')+t('Současný kandidát v4 aktualizuje paměť na pozadí hned po přijetí repliky. Luna rozlišila osobu v baru od zahradního nářadí i po odbočkách a změně tématu. Serverová Gemma při jedné změně zůstala u starého významu. První blokující pokus nechal klíčovou větu vypršet ve frontě, ačkoli ji OCR přečetlo; i tento neúspěch je v datech.','The current v4 candidate updates memory in the background immediately after accepting a line. Luna distinguished the person at the bar from the garden tool after filler and a topic change. Server Gemma retained the old meaning on one topic change. The first blocking attempt let a key line expire in the queue even though OCR read it; that failure remains in the data.','p')+t('Jde o vývojový pilot jednoho dvojznačného slova, nikoli univerzální zlepšení překladů. Funkce zůstává volitelná a vypnutá. Širších 25 českých a 50 vícejazyčných ukázek má vzniknout až z ověřeného nasazení této cesty, nikoli z ručně vloženého kontextu do API. Veřejný balíček preview.18 tuto novou funkci ještě neobsahuje.','This is a development pilot of one ambiguous word, not universal translation improvement. The feature is optional and defaults off. The larger 25 Czech and 50 multilingual examples must come from this verified application path, not manually supplied API context. The public preview.18 package does not yet contain this new feature.','p')+'</section>'
 body+='<section class="section">'+t('Co změnily další skutečné běhy','What further actual-app runs changed','h2')+t('Aktualizace nyní začíná hned po přijetí repliky, aby mohla být hotová před následujícím titulkem. V3 odhalila další chybu: Gemma zaměnila interní číslo události 69 za ID věty o69, které neexistovalo. Paměť se správně zastavila a zůstala krátká historie. V4 tato nadbytečná čísla modelu neposílá. Ve čtyřech bězích v4 se chyba odkazu neopakovala a všech dvanáct cílových výskytů bylo zobrazeno. Neznamená to záruku bezchybné paměti.','Updates now start immediately after a caption is accepted so they can finish before the next subtitle. V3 exposed another failure: Gemma confused internal event number 69 with nonexistent source-line ID o69. Memory correctly stopped and fell back to recent history. V4 removes those redundant numbers from model input. All four v4 runs avoided citation errors and displayed all twelve target occurrences. This does not guarantee error-free memory.','p')+t('Luna ve v4 zachovala správný smysl všech šesti cílů; jednou mírně změnila příkaz. Gemma v barové scéně stále ignorovala výslovný přesun na zahradu, přestože nové věty dostala, a měla i chyby češtiny. Delší paměť proto nedoporučujeme plošně: u Luny je kandidátem na širší ověření, u testované serverové Gemmy zatím dej přednost krátké historii. Lepší odpověď po automatickém vypnutí paměti ve v3 nepočítáme jako úspěch této funkce.','In v4 Luna preserved the intended sense of all six targets, with one minor imperative shift. In the bar scene Gemma still ignored an explicit move to the garden despite receiving the new lines, and also made Czech-language errors. Longer memory is therefore not a blanket recommendation: Luna merits wider validation; prefer recent history for this tested server Gemma. A better answer after v3 automatically disabled memory does not count as a success of this feature.','p')+t('Serverová Gemma 4 26B A4B Q5_K_S běží na samostatném počítači s RTX 3090 24 GB. Notebook RTX 4050 zpracovává obraz, OCR, VFG a NR. Do serveru se posílají pouze textové požadavky; síťová prodleva je zahrnutá v měření.','Server Gemma 4 26B A4B Q5_K_S runs on a separate RTX 3090 24 GB computer. The RTX 4050 laptop handles image processing, OCR, VFG and NR. Only text requests go to the server, and network delay is included in the measurements.','p')+'</section>'
 body+='<section class="section"><details><summary>'+t('Jak jsme vybírali metodu: další oddělené textové zkoušky','How the method was selected: separate text-only checks')+'</summary>'+t('Po původním testu následovalo 40 oddělených ověřovacích replik (400 výstupů) a dalších 24 potvrzovacích (192 výstupů). Codex posoudil anonymně označené odpovědi před odhalením modelu a metody. Tabulka ukazuje poslední sadu: přepočet skóre z přímého posouzení na 0–100, nikoli procentní přesnost. Jde o malé navazující úlohy; vybraná paměť nepřekonala běžných šest replik celkově. Její přínos hledáme v uchování vodítka přes jiné hlášky.','After the initial experiment came 40 separate validation captions (400 outputs) and 24 additional confirmation captions (192 outputs). Codex assessed anonymously labelled responses before revealing model/method identities. This table shows the latter set: direct-review scores scaled to 0–100, not accuracy percentages. These are small, clustered tasks; selected memory did not beat ordinary six-line history overall. Its specific purpose is retaining clues through intervening chatter.','p')+'<div class="table-wrap"><table><thead><tr>'+t('Model','Model','th')+t('Metoda','Method','th')+t('Skóre 0–100','Score 0–100','th')+'</tr></thead><tbody>'
 methods={'baseline':('Bez kontextu','No context'),'raw6':('Posledních šest replik','Last six lines'),'raw16':('Posledních šestnáct replik','Last sixteen lines'),'retrieved':('Vybrané původní repliky','Selected original lines')}
 for row in json.loads((source/'confirmation-summary.json').read_text(encoding='utf-8')):
  body+='<tr><td>'+html.escape(row['model'])+'</td>'+t(*methods[row['mode']],'td')+f'<td>{row["score"]:.2f}</td></tr>'
 body+='</tbody></table></div><p><a href="data/selection-review/validation-assessments.json">'+t('400 posouzených výstupů','400 assessed outputs')+'</a> · <a href="data/selection-review/confirmation-assessments.json">'+t('192 potvrzovacích výstupů','192 confirmation outputs')+'</a></p></details></section>'
 body+='<section class="section" id="outputs">'+t('Stejná věta, skutečně zobrazené výsledky','The same line, actually displayed results','h2')+'<blockquote>Get that hoe out of here.</blockquote>'+t('V každé scéně: úvodní význam → 20 odboček včetně SPZ a nápisu → stejná věta → výslovná změna tématu → stejná věta. Tabulka obsahuje výhradně titulky zachycené událostí caption_display v aplikaci.','Each scene: initial meaning → 20 filler lines including a plate and label → the same line → explicit topic change → the same line. The table contains only captions confirmed by native caption_display events.','p')+'<label><input id="show-old-attempts" type="checkbox">'+t('Zobrazit i starší vývojové pokusy (v1–v3)','Also show earlier development attempts (v1–v3)')+'</label><div class="table-wrap"><table id="live-runs-table"><thead><tr>'+''.join(t(cs,en,'th') for cs,en in [('Model / režim','Model / mode'),('Začátek','Initially'),('Po odbočkách','After filler'),('Po změně tématu','After topic change'),('Prodleva / FPS','Latency / FPS')])+'</tr></thead><tbody>'
 for r in runs:
  model='Luna' if r['provider']=='openai' else 'Gemma · RTX 3090'
  modes={'off':('bez paměti','memory off'),'recent':('krátká historie','recent history'),'selected':('výběr původních replik','selected original lines')}
  cs,en=modes[r['mode']];cs+=' '+r['variant'];en+=' '+r['variant']
  if r['synchronous']:cs='blokující prototyp v1';en='synchronous prototype v1'
  old=r['mode']=='selected' and r['variant']!='v4'
  attrs=' data-old-attempt="true" hidden' if old else ''
  body+='<tr'+attrs+'><th><a href="#'+r['id']+'">'+t(model+' / '+('zahrada' if r['scene']=='garden' else 'bar')+' / '+cs,model+' / '+r['scene']+' / '+en)+'</a></th>'
  for target in r['target_review']:
   body+='<td>'+(html.escape(target['output']) if target['output'] is not None else t('NEZOBRAZENO','NOT DISPLAYED'))+f'<br><small>{target["score"]}/4</small>'+ (t(' · PAMĚŤ ZASTAVENA',' · MEMORY STOPPED','small') if target['memory_status'].startswith('Memory stopped') else '')+'</td>'
  body+=f'<td>{r["display_latency_ms"]["median"]/1000:.2f} s<br>{r["metrics"]["output_submitted_fps"]["p50"]:.1f} FPS</td></tr>'
 body+='</tbody></table></div>'+t('Prodleva = medián od zachycení obrazu do zobrazení pro všechny zobrazené titulky daného běhu. FPS = medián skutečně odeslaných snímků aplikace; není to měření panelu monitoru. Skóre 0–4 hodnotí jen tři cílové výskyty: 4 správný význam a čitelná čeština, 3 drobná jazyková chyba/posun, 2 důležitý posun, 1 chybný hlavní význam, 0 nezobrazená věta. Není to skóre celého dialogu nebo modelu.','Latency is the median capture-to-display delay across all displayed captions in that run. FPS is median app-submitted content, not a measurement of the monitor panel. Scores 0–4 assess only the three target occurrences: 4 correct meaning/readable Czech, 3 minor language error/shift, 2 important shift, 1 wrong main meaning, 0 not displayed. This is not a full-dialogue or overall-model score.','p')+'</section>'
 shot='data/live/openai-bar-selected-async-v2/delayed-caption.jpg'
 body+='<p>'+t('V1 čeká na aktualizaci paměti. V2 ji spouští na pozadí při následující replice. V3 ji spouští hned po přijetí nové repliky. V4 navíc odstraní interní čísla událostí ze vstupu modelu; zůstávají pouze ID původních vět. Výchozí tabulka ukazuje v4 a kontrolní běhy. Starší verze lze zobrazit zaškrtnutím.', 'V1 waits for memory updates. V2 starts them in the background when the next line arrives. V3 starts them immediately after accepting a new line. V4 also removes internal event numbers from model input, leaving only original-line IDs. The default table shows v4 and controls; check the box for earlier versions.')+'</p>'
 body+='<section class="section">'+t('Skutečný screenshot z běhu','Actual screenshot from the run','h2')+f'<a href="{shot}"><img src="{shot}" style="width:100%;height:auto" alt="FrameLab: Get that hoe out of here. / Dostaňte tu couru odsud."></a>'+t('Původní screenshot okna; text ani obraz nejsou dodatečně rekonstruované. Zachycuje opožděnou repliku v barové scéně, VFG a zapnutý NR.','Original window screenshot; neither text nor picture is reconstructed. It captures the delayed bar-scene target with VFG and NR enabled.','p')+'</section>'
 body+='<section class="section" id="method">'+t('Co se měřilo a co z toho neplyne','Method and limits','h2')+t('RTX 4050 Laptop 6 GB, i5-13420H, 32 GB RAM. Zdroj 1280 × 720 / 30 FPS, VFG 2×, NR 540p / jeden průchod, RapidOCR každých 300 ms. Anglické autorské repliky jsou vypálené do opakované existující ukázky GTA; jde o řízenou simulaci, nikoli dialog z této hry. Přepis se do překladače nevkládal. Překlad i výběr paměti dělal tentýž zvolený model.','RTX 4050 Laptop 6 GB, i5-13420H, 32 GB RAM. Source 1280 × 720 / 30 FPS, VFG 2×, NR 540p / one pass, RapidOCR every 300 ms. Authored English lines are burned into an existing repeated GTA sample: a controlled simulation, not that game’s dialogue. No transcript was injected into translation. The same selected model handled translation and memory selection.','p')+t('Luna používá skutečné produkční promptování včetně chytrého filtru, thinking none a výchozí neposílané teploty. Server má teplotu 0 a vypnuté uvažování. Každá buňka je jeden běh; pořadí není náhodné a síťová prodleva kolísá. Bez paměti běží běžná cache překladů, takže opakovanou větu může chybně vrátit i po změně tématu. Paměť cache obchází. To vše je součást aplikace, nikoli izolovaný benchmark modelu.','Luna uses actual production prompting including the smart filter, thinking none and default omitted temperature. Server temperature is zero with thinking disabled. Each cell is one run; order is not randomized and network latency varies. With memory off, normal translation caching can wrongly reuse a repeated line after a topic change. Memory bypasses this cache. These are application behaviours, not an isolated model benchmark.','p')+t('Metoda uchovává původní citované repliky, ne tvrzení vygenerovaná AI. Aktualizuje nejvýše jednou za osm sekund, pracuje na pozadí a staré výsledky po změně zdroje zahazuje. Slabší model může vybrat špatné věty nebo správné věty ignorovat. CUNI tvorbu této paměti nezvládlo. Doporučení zůstává závislé na modelu; žádný placený hodnotitel se nepoužil.','The method retains cited original lines, not AI-generated claims. It updates at most once per eight seconds in the background and discards stale results after source changes. A weaker model can select the wrong lines or ignore the right ones. CUNI failed the memory-building task. Recommendations remain model-dependent; no paid judge was used.','p')+'</section>'
 body+='<section class="section" id="all-runs">'+t('Všechny běhy a použité repliky','Every run and its actual context','h2')
 for r in runs:
  body+=f'<details class="case" id="{r["id"]}"><summary>{html.escape(r["id"])}</summary><p>'+t(f'Zobrazeno {r["rendered_count"]} titulků. Záznamů zahození/vypršení: {len(r["drops"])}.',f'{r["rendered_count"]} captions displayed. Drop/expiry events: {len(r["drops"])}.')+'</p>'
  if r['memory_errors']:body+='<p class=notice>'+t('Tvorba paměti se zastavila kvůli neplatnému odkazu. Následující výsledky používají krátkou historii; nejsou důkazem přínosu dlouhodobé paměti.','Memory generation stopped on an invalid citation. Subsequent results use recent history; they are not evidence of long-term memory benefit.')+'</p>'
  if r['cost_usd'] is not None:body+='<p>'+t('Odhad API včetně paměti: ','Estimated API cost including memory: ')+f'${r["cost_usd"]:.6f} · {r["completed_api_calls"]} '+t('dokončených volání s usage.','completed calls with usage.')+'</p>'
  worker={x.get('event_id'):x for x in r['worker']}
  for row in r['displayed']:
   body+='<details><summary>'+html.escape(row['source'])+f' · {row["capture_to_display_ms"]/1000:.2f} s</summary><blockquote>'+html.escape(row['translation'])+'</blockquote>'
   context=worker.get(str(row['job_id']),{}).get('context')
   if context:
    body+='<ul>'+''.join('<li>'+html.escape(x['source'])+f' <small>({x["seconds_since_ocr_observation"]:.1f} s)</small></li>' for x in context.get('history',[]))+'</ul>'
   body+='</details>'
  body+=f'<p><a href="data/live/{r["id"]}/worker.jsonl">'+t('Paměť a odpovědi pracovníka','Worker memory and responses')+f'</a> · <a href="data/live/{r["id"]}/events.jsonl">'+t('Skutečné OCR a zobrazení','Actual OCR and display events')+'</a></p></details>'
 body+='</section><p><a href="live-evidence.zip">'+t('Reprodukční data a zmrazený kód · ZIP','Reproduction data and frozen code · ZIP')+'</a> · <a href="speed50.html">'+t('50 + 50: rychlost serveru a API','50 + 50: server/API speed')+'</a> · <a href="./">'+t('Předběžné textové experimenty','Preliminary text-only experiments')+'</a> · <a href="../../guide/custom-ai-en.html">'+t('Vlastní AI server','Custom AI server')+'</a></p>'
 body+='<script>document.getElementById("show-old-attempts").addEventListener("change",e=>document.querySelectorAll("tr[data-old-attempt]").forEach(row=>row.hidden=!e.target.checked));</script>'
 head='<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">'+t('FrameLab — paměť v běžící aplikaci','FrameLab — live conversation memory','title')
 for f in ['assets/fonts/fonts.css','explore.css','site-i18n.css','site-navigation.css']:head+=f'<link rel="stylesheet" href="../../{f}">'
 for f in ['site-i18n.js','site-navigation.js']:head+=f'<script defer src="../../{f}"></script>'
 head+='<style>td{min-width:180px}td small{color:var(--muted)}details details{padding:12px;border-top:1px solid var(--line)}summary{cursor:pointer}pre{overflow:auto}</style>'
 (dest/'live.html').write_text('<!doctype html><html lang="en" data-site-page="runs/2026-11-24-context-memory/live.html"><head>'+head+'</head><body><main>'+body+'</main></body></html>',encoding='utf-8')
 with zipfile.ZipFile(dest/'live-evidence.zip','w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for f in data.rglob('*'):
   if f.is_file():z.write(f,'data/'+str(f.relative_to(data)))
  for label in ['live-frozen-v1','live-frozen-v2','live-frozen-v3','live-frozen-v4']:
   for f in (source/label).rglob('*'):
    if f.is_file():z.write(f,label+'/'+str(f.relative_to(source/label)))
 print('Built live report:',len(runs),'runs;',sum(len(r['target_review']) for r in runs),'reviewed display opportunities.')

if __name__=='__main__':main()
