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
  actual=[by_id.get(x['job_id'],{}).get('translation') for x in targets];assert actual==expected,(name,actual,expected)
  reviewed=[dict(phase=i,job_id=r['job_id'],output=actual[i],score=grades[i],display_ms=by_id.get(r['job_id'],{}).get('capture_to_display_ms'),output_sha256=hashlib.sha256((actual[i] or '').encode()).hexdigest()) for i,r in enumerate(targets)]
  metrics=json.loads((folder/'summary.json').read_text(encoding='utf-8'))['metrics'];cost=None;usage=[]
  if (folder/'api-usage.jsonl').exists():
   usage=read_lines(folder/'api-usage.jsonl');cost=0.
   for row in usage:
    u=row['usage'];d=u.get('input_tokens_details',{});c=d.get('cached_tokens',0);w=d.get('cache_creation_tokens',d.get('cache_write_tokens',0));cost+=(max(0,u['input_tokens']-c-w)*.10+c*.01+w*.125+u['output_tokens']*.50)/1e6
  run=dict(id=name,provider=meta['provider'],scene=meta['scene'],mode=meta['mode'],synchronous=name=='openai-bar-selected',rendered_count=len(shown),display_latency_ms=stats([x['capture_to_display_ms'] for x in shown]),target_review=reviewed,metrics=metrics,cost_usd=cost,completed_api_calls=len(usage),displayed=shown,drops=[x for x in events if x['event'] in ['translation_dropped','translation_expired','caption_expired']],worker=read_lines(folder/'worker.jsonl'))
  runs.append(run);(data/name).mkdir(exist_ok=True)
  for f in folder.iterdir():
   if f.suffix in ['.json','.jsonl','.csv','.png','.jpg']:shutil.copy2(f,data/name/f.name)
 report=dict(scope='Live native-app pilot: two authored scenes, 11 runs. Not the requested future 25 Czech + 50 multilingual showcase. No paid judge. Target scores are direct unblinded Codex review of 33 render opportunities, not full-dialogue quality or population accuracy.',runs=runs)
 (data/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 for name in ['bar.json','garden.json','bar.ass','garden.ass']:shutil.copy2(source/'live-scenes'/name,data/name)
 body='<header class="page-intro"><span class="eyebrow">FRAME/LAB · LIVE CONTEXT PILOT</span>'+t('Co paměť udělá ve skutečném programu?','What does memory do in the actual app?','h1')+t('Obraz → OCR → filtr → vybraný model a jeho vlastní paměť → zobrazený titulek. Dvě scény, 11 skutečných běhů.','Picture → OCR → filter → selected model and its own memory → displayed caption. Two scenes, 11 real runs.','p','lead')+'</header>'
 body+='<section class="section">'+t('Výsledek i selhání','Results and failures','h2')+t('Aktualizace paměti musí běžet na pozadí. První synchronní pokus nechal klíčovou větu vypršet ve frontě, ačkoli ji OCR přečetlo. Druhá varianta u Luny zobrazila všechny klíčové věty a rozlišila osobu v baru od zahradního nářadí i po odbočkách a změně tématu. Serverová Gemma někdy zůstala u starého významu.','Memory updates need to run in the background. The first synchronous attempt let a key line expire in the queue even though OCR read it. With Luna, the second version displayed all key lines and distinguished the person at the bar from the garden tool after filler and a topic change. Server Gemma sometimes retained the old meaning.','p')+t('Jde o vývojový pilot jednoho dvojznačného slova, nikoli univerzální zlepšení překladů. Funkce zůstává volitelná a vypnutá. Širších 25 českých a 50 vícejazyčných ukázek má vzniknout až z ověřeného nasazení této cesty, nikoli z ručně vloženého kontextu do API. Veřejný balíček preview.18 tuto novou funkci ještě neobsahuje.','This is a development pilot of one ambiguous word, not universal translation improvement. The feature is optional and defaults off. The larger 25 Czech and 50 multilingual examples must come from this verified application path, not manually supplied API context. The public preview.18 package does not yet contain this new feature.','p')+'</section>'
 body+='<section class="section">'+t('Stejná věta, skutečně zobrazené výsledky','The same line, actually displayed results','h2')+'<blockquote>Get that hoe out of here.</blockquote>'+t('V každé scéně: úvodní význam → 20 odboček včetně SPZ a nápisu → stejná věta → výslovná změna tématu → stejná věta. Tabulka obsahuje výhradně titulky zachycené událostí caption_display v aplikaci.','Each scene: initial meaning → 20 filler lines including a plate and label → the same line → explicit topic change → the same line. The table contains only captions confirmed by native caption_display events.','p')+'<div class="table-wrap"><table><thead><tr>'+''.join(t(cs,en,'th') for cs,en in [('Model / režim','Model / mode'),('Začátek','Initially'),('Po odbočkách','After filler'),('Po změně tématu','After topic change'),('Prodleva / FPS','Latency / FPS')])+'</tr></thead><tbody>'
 for r in runs:
  label=('Luna' if r['provider']=='openai' else 'Gemma · RTX 3090')+' / '+r['scene']+' / '+('sync prototype' if r['synchronous'] else r['mode'])
  body+='<tr><th><a href="#'+r['id']+'">'+html.escape(label)+'</a></th>'
  for target in r['target_review']:
   body+='<td>'+(html.escape(target['output']) if target['output'] is not None else t('NEZOBRAZENO','NOT DISPLAYED'))+f'<br><small>{target["score"]}/4</small></td>'
  body+=f'<td>{r["display_latency_ms"]["median"]/1000:.2f} s<br>{r["metrics"]["output_submitted_fps"]["p50"]:.1f} FPS</td></tr>'
 body+='</tbody></table></div>'+t('Prodleva = medián od zachycení obrazu do zobrazení pro všechny zobrazené titulky daného běhu. FPS = medián skutečně odeslaných snímků aplikace; není to měření panelu monitoru. Skóre 0–4 hodnotí jen tři cílové výskyty: 4 správný význam a čitelná čeština, 3 drobná jazyková chyba/posun, 2 důležitý posun, 1 chybný hlavní význam, 0 nezobrazená věta. Není to skóre celého dialogu nebo modelu.','Latency is the median capture-to-display delay across all displayed captions in that run. FPS is median app-submitted content, not a measurement of the monitor panel. Scores 0–4 assess only the three target occurrences: 4 correct meaning/readable Czech, 3 minor language error/shift, 2 important shift, 1 wrong main meaning, 0 not displayed. This is not a full-dialogue or overall-model score.','p')+'</section>'
 shot='data/live/openai-bar-selected-async-v2/delayed-caption.jpg'
 body+='<section class="section">'+t('Skutečný screenshot z běhu','Actual screenshot from the run','h2')+f'<a href="{shot}"><img src="{shot}" style="width:100%;height:auto" alt="FrameLab: Get that hoe out of here. / Dostaňte tu couru odsud."></a>'+t('Původní screenshot okna; text ani obraz nejsou dodatečně rekonstruované. Zachycuje opožděnou repliku v barové scéně, VFG a zapnutý NR.','Original window screenshot; neither text nor picture is reconstructed. It captures the delayed bar-scene target with VFG and NR enabled.','p')+'</section>'
 body+='<section class="section">'+t('Co se měřilo a co z toho neplyne','Method and limits','h2')+t('RTX 4050 Laptop 6 GB, i5-13420H, 32 GB RAM. Zdroj 1280 × 720 / 30 FPS, VFG 2×, NR 540p / jeden průchod, RapidOCR každých 300 ms. Anglické autorské repliky jsou vypálené do opakované existující ukázky GTA; jde o řízenou simulaci, nikoli dialog z této hry. Přepis se do překladače nevkládal. Překlad i výběr paměti dělal tentýž zvolený model.','RTX 4050 Laptop 6 GB, i5-13420H, 32 GB RAM. Source 1280 × 720 / 30 FPS, VFG 2×, NR 540p / one pass, RapidOCR every 300 ms. Authored English lines are burned into an existing repeated GTA sample: a controlled simulation, not that game’s dialogue. No transcript was injected into translation. The same selected model handled translation and memory selection.','p')+t('Luna používá skutečné produkční promptování včetně chytrého filtru, thinking none a výchozí neposílané teploty. Server má teplotu 0 a vypnuté uvažování. Každá buňka je jeden běh; pořadí není náhodné a síťová prodleva kolísá. Bez paměti běží běžná cache překladů, takže opakovanou větu může chybně vrátit i po změně tématu. Paměť cache obchází. To vše je součást aplikace, nikoli izolovaný benchmark modelu.','Luna uses actual production prompting including the smart filter, thinking none and default omitted temperature. Server temperature is zero with thinking disabled. Each cell is one run; order is not randomized and network latency varies. With memory off, normal translation caching can wrongly reuse a repeated line after a topic change. Memory bypasses this cache. These are application behaviours, not an isolated model benchmark.','p')+t('Metoda uchovává původní citované repliky, ne tvrzení vygenerovaná AI. Aktualizuje nejvýše jednou za osm sekund, pracuje na pozadí a staré výsledky po změně zdroje zahazuje. Slabší model může vybrat špatné věty nebo správné věty ignorovat. CUNI tvorbu této paměti nezvládlo. Doporučení zůstává závislé na modelu; žádný placený hodnotitel se nepoužil.','The method retains cited original lines, not AI-generated claims. It updates at most once per eight seconds in the background and discards stale results after source changes. A weaker model can select the wrong lines or ignore the right ones. CUNI failed the memory-building task. Recommendations remain model-dependent; no paid judge was used.','p')+'</section>'
 body+='<section class="section">'+t('Všechny běhy a použité repliky','Every run and its actual context','h2')
 for r in runs:
  body+=f'<details class="case" id="{r["id"]}"><summary>{html.escape(r["id"])}</summary><p>'+t(f'Zobrazeno {r["rendered_count"]} titulků. Záznamů zahození/vypršení: {len(r["drops"])}.',f'{r["rendered_count"]} captions displayed. Drop/expiry events: {len(r["drops"])}.')+'</p>'
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
 head='<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">'+t('FrameLab — paměť v běžící aplikaci','FrameLab — live conversation memory','title')
 for f in ['assets/fonts/fonts.css','explore.css','site-i18n.css','site-navigation.css']:head+=f'<link rel="stylesheet" href="../../{f}">'
 for f in ['site-i18n.js','site-navigation.js']:head+=f'<script defer src="../../{f}"></script>'
 head+='<style>td{min-width:180px}td small{color:var(--muted)}details details{padding:12px;border-top:1px solid var(--line)}summary{cursor:pointer}pre{overflow:auto}</style>'
 (dest/'live.html').write_text('<!doctype html><html lang="en" data-site-page="runs/2026-11-24-context-memory/live.html"><head>'+head+'</head><body><main>'+body+'</main></body></html>',encoding='utf-8')
 with zipfile.ZipFile(dest/'live-evidence.zip','w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for f in data.rglob('*'):
   if f.is_file():z.write(f,'data/'+str(f.relative_to(data)))
  for label in ['live-frozen-v1','live-frozen-v2']:
   for f in (source/label).rglob('*'):
    if f.is_file():z.write(f,label+'/'+str(f.relative_to(source/label)))
 print('Built live report:',len(runs),'runs;',sum(len(r['target_review']) for r in runs),'reviewed display opportunities.')

if __name__=='__main__':main()
