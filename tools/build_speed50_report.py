"""Publish measured speed data without inventing translation-quality scores."""
from pathlib import Path
import argparse,html,json,shutil
from build_memory_report import t

def main():
 p=argparse.ArgumentParser();p.add_argument('source',type=Path);a=p.parse_args();source=a.source
 dest=Path(__file__).resolve().parents[1]/'website/runs/2026-11-24-context-memory';data=dest/'data/speed50';data.mkdir(parents=True,exist_ok=True)
 meta=json.loads((source/'metadata.json').read_text(encoding='utf-8'));rows=[json.loads(x) for x in (source/'outputs.jsonl').read_text(encoding='utf-8').splitlines()]
 assert meta['completed']==100 and len(rows)==100
 for f in ['metadata.json','outputs.jsonl']:shutil.copy2(source/f,data/f)
 body='<header class="page-intro"><span class="eyebrow">50 + 50 · 24 / 11 / 2026</span>'+t('Tvoje Gemma na serveru, nebo OpenAI?','Your server Gemma, or OpenAI?','h1')+t('Stejných 50 titulků. Střídání serveru a API. Čas až do celé odpovědi.','The same 50 captions. Alternating server/API requests. Time until the complete response.','p','lead')+'</header>'
 body+='<div class="cards">'
 for provider,label in [('server','Gemma 4 26B A4B · RTX 3090'),('openai','GPT-6 Luna · OpenAI')]:
  s=meta['summary'][provider]['latency_ms'];body+='<article class="card"><h2>'+label+'</h2><div class="number">'+f'{s["median"]/1000:.2f} s</div>'+t('Medián celého požadavku','Median full-request latency','p')+'<p>P95: '+f'{s["p95"]/1000:.2f} s</p></article>'
 body+='<article class="card"><div class="number">'+str(meta['pair_wins']['server'])+'/50</div>'+t('dvojic vyhrál server rychlostí','pairs won by the server on speed','p')+'<p>OpenAI: $'+f'{meta["api_cost_usd"]:.6f}'+'</p>'+t('Pouze překlady. Žádný placený hodnotitel.','Translations only. No paid judge.','p')+'</article></div>'
 body+='<section class="section">'+t('Co tento test říká','What this test establishes','h2')+t('V tomto běhu byl medián poměru časů jednotlivých dvojic 3,51× ve prospěch serveru. Obě strany dokončily 50/50 požadavků bez chyb. Výsledek platí pro zdejší síť, běžící server a toto nastavení; neznamená lepší kvalitu češtiny.','The median per-pair time ratio was 3.51× in favour of the server. Both completed 50/50 requests without errors. This describes this network, running server and configuration; it does not establish better Czech translation quality.','p')+t('EN → CS, bez historie, OCR, frame genu a enhanceru. Myšlení vypnuté, teplota 0, limit 256 výstupních tokenů, timeout 12 s. V 25 dvojicích šel první server, v 25 OpenAI. Jednotlivé požadavky se nepřekrývaly. Serverový model už byl načtený; první požadavek je ve výsledcích ponechaný.','EN → CS, with history, OCR, frame generation and enhancement off. Thinking off, temperature zero, 256 output-token limit, 12-second timeout. Server first in 25 pairs, OpenAI first in 25. Requests did not overlap. The server model was already resident; the first request remains in the results.','p')+t('Použité adaptéry nejsou totožné: OpenAI Responses vrací strukturované JSON, server Chat Completions vrací samotný text. Měříme praktické zpoždění těchto dvou cest, nikoli izolovaný výpočet GPU nebo čas do prvního tokenu. Vstupy jsou náhodně zvolených 50 unikátních hodnotitelných vět z předchozího testu; server mohl využívat cache promptu.','Adapters differ: OpenAI Responses returns structured JSON, while server Chat Completions returns plain text. This is practical latency for these two paths, not isolated GPU compute or time to first token. Inputs are a seeded random selection of 50 unique assessable lines from the previous test; the server could benefit from prompt caching.','p')+'</section>'
 body+='<section class="section">'+t('Všech 50 dvojic','All 50 pairs','h2')
 for pair in range(1,51):
  values={r['provider']:r for r in rows if r['pair']==pair};body+='<details class="case"><summary>'+str(pair)+'. '+html.escape(values['server']['source'])+'</summary><div class="case pair">'
  for provider,label in [('server','Gemma · RTX 3090'),('openai','GPT-6 Luna')]:
   r=values[provider];body+='<article><h3>'+label+' · '+f'{r["ms"]/1000:.2f} s</h3><blockquote>'+html.escape(r['translation'])+'</blockquote></article>'
  body+='</div></details>'
 body+='</section><div class="download-links"><a href="data/speed50/metadata.json">Metadata / JSON</a><a href="data/speed50/outputs.jsonl">'+t('Všech 100 nezměněných odpovědí a časů','All 100 unmodified responses and timings')+'</a><a href="./">'+t('Test paměti a kvalita překladů','Memory test and translation quality')+'</a></div>'
 head='<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">'+t('FrameLab — Gemma versus OpenAI: 50 + 50','FrameLab — Gemma versus OpenAI: 50 + 50','title')
 for f in ['assets/fonts/fonts.css','explore.css','site-i18n.css','site-navigation.css']:head+=f'<link rel="stylesheet" href="../../{f}">'
 for f in ['site-i18n.js','site-navigation.js']:head+=f'<script defer src="../../{f}"></script>'
 (dest/'speed50.html').write_text('<!doctype html><html lang="en" data-site-page="runs/2026-11-24-context-memory/speed50.html"><head>'+head+'</head><body><main>'+body+'</main></body></html>',encoding='utf-8');print('Speed50 report generated')

if __name__=='__main__':main()
