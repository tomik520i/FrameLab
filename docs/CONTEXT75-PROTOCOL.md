# 75-scene application evaluation / Ověření aplikace na 75 scénách

This document describes the fixed protocol, not a promise of translation quality.
Results, completion state, original pictures and per-case judgements are in the
[CS/EN interactive report](https://opicebot.cz/framelab/runs/2026-11-24-context75/).
The study publishes source and evidence; the public installation package remains
preview.18. The measured development executable is identified by SHA-256.

Tento dokument popisuje pevný postup měření, nikoli záruku kvality překladu.
Stav dokončení, výsledky, původní obrázky a jednotlivé posudky jsou ve
[výsledcích česky i anglicky](https://opicebot.cz/framelab/runs/2026-11-24-context75/?lang=cs).
Studie zveřejňuje kód a důkazy. Veřejný instalační balíček zůstává preview.18;
měřený vývojový program je určen kontrolním součtem SHA-256.

## Fixed comparison / Pevně dané srovnání

| Item / Položka | Definition / Vymezení |
| --- | --- |
| Cases / Případy | 25 EN→CS; 5 each / po pěti EN→JA, EN→DE, EN→ES, EN→FR, CS→JA, JA→EN, JA→CS, DE→EN, ES→EN, FR→EN |
| Runs / Běhy | 75 × 2 providers × 2 memory modes = 300 |
| Models / Modely | gpt-6-luna, effort none; LAN Gemma 4 26B A4B ultra-uncensored-heretic Q5_K_S |
| Memory / Paměť | Off versus v4 asynchronous selection of original dialogue / vypnuto proti asynchronnímu výběru původních replik v4 |
| Targets / Cíle | A at 9–13 s, A after filler at 23–27 s, B after topic change at 33–38 s |
| Proofs / Obrazové důkazy | Native framebuffer at 12.5, 26.5 and 37.5 s, with Present result / skutečný obrazový buffer a výsledek Present |
| Processing / Zpracování | Native video decoder → OCR → filter → provider → caption queue → native renderer |
| Graphics / Grafika | VFG 2× + NR 540p, one pass / jeden průchod, strength / síla 0.65 |
| Laptop / Notebook | i5-13420H, RTX 4050 Laptop 6 GB, 32 GB RAM, Windows 11 |
| LAN server | Separate RTX 3090 24 GB / samostatná RTX 3090 24 GB |

There are 50 unique source videos because some source scenes are translated into
several languages. The dialogues are authored diagnostics over a short repeating
owner-recorded gameplay clip. They are not original GTA dialogue or a random
sample of all games. Eight 1.25-second interruptions include conversation, a
plate and a UI label. A UI label can also be a valid imperative; translating it
is not automatically a false positive.

Existuje 50 různých zdrojových videí, protože některé scény překládáme do více
jazyků. Dialogy jsou autorská diagnostika nad krátkým opakovaným herním záznamem
majitele projektu. Nejsou to původní dialogy GTA ani náhodný výběr všech her.
Osm odboček po 1,25 sekundy zahrnuje dialog, SPZ a text rozhraní. Text rozhraní
může být i platný rozkaz; jeho přeložení samo o sobě neprokazuje chybu filtru.

## What counts / Co se počítá

Primary outcome: the caption visible at the fixed proof instant has the correct
meaning and usable language. Blank output, an earlier caption and an incorrect
sense stay in the denominator. This measures the complete application. A
separate first target-result view preserves late answers and separates language
quality from delivery. Timing includes OCR, queueing, model inference and display
waiting. Missing displays are excluded from latency percentiles, never from
failure counts. Source-onset timing is estimated from native video/proof anchors;
capture-to-display latency is recorded by the application.

Hlavní výsledek: v určeném okamžiku je skutečně vidět správný význam s použitelným
jazykem. Prázdný obraz, předchozí titulek a nesprávný význam zůstávají v jmenovateli.
Hodnotí se celá aplikace. Samostatný první výsledek cílového požadavku uchovává
i pozdě doručené odpovědi a odděluje jazyk od doručení. Prodleva zahrnuje OCR,
frontu, model i čekání na zobrazení. Nezobrazené výstupy chybějí v percentilech
prodlevy, ale zůstávají v neúspěších. Čas od začátku repliky se odhaduje podle
videových kotev; čas od zachycení po zobrazení přímo zaznamenává aplikace.

Codex directly assesses meaning (0/1/2) and language (0/1/2), with provider and
mode omitted from the candidate file. Identical outputs within a case/context
are deduplicated. References illustrate meaning; they are not exact-match tests.
A neutral expression valid in both contexts receives full credit. This is AI
qualitative review, not a panel of native speakers. No paid judging API is used.

Codex přímo posuzuje význam (0/1/2) a jazyk (0/1/2). Soubor pro posouzení neuvádí
model ani režim. Shodné výstupy v témže případu a kontextu se hodnotí jednou.
Reference ukazují význam, nejsou test přesné shody. Neutrální výraz správný v obou
kontextech dostane plné hodnocení. Jde o kvalitativní posudek AI, nikoli panel
rodilých mluvčích. Hodnoticí API se nevolá ani neplatí.

## Limits and reproducibility / Omezení a opakování

- One run per arm and case; rotating arm order and seeded case shuffle. No
  confidence claim based on repeated independent trials.
- Memory-off retains ordinary caching. Memory-on re-translates with context;
  initially it can use recent history while selection runs. The comparison
  therefore does not isolate long memory from short history or cache effects.
- The selected provider builds its own memory. No other model prepares context.
  Invalid evidence IDs stop the long-memory builder and retain short history.
- The model on the LAN server is a Heretic derivative, not an unmodified Google
  release. Its weights do not run on the laptop.
- Ordinary background applications remain open. Any overlapping auxiliary upload
  is disclosed in `auxiliary-activity.json`. No changes to inference code, model
  parameters, source scenes or timing are made inside the main batch.
- PNG proofs preserve the original BMP RGB pixels exactly. Readback stalls
  rendering briefly; the logs retain its duration. Present success does not
  establish what a physical display panel emitted.
- Input scenes, original logs, hashes and frozen source are downloadable from
  the report. Runtime DLLs, credentials, proprietary weights and the measured
  executable are not redistributed in the reproduction archive.

Každý případ má jeden běh každé varianty. Pořadí variant se střídá a případy jsou
předem zamíchané. Běžná cache zůstává součástí režimu bez paměti; zapnutá paměť
znovu překládá a zpočátku může použít krátkou historii. Studie tedy neodděluje
přínos dlouhé paměti od krátké historie a cache. Paměť vytváří sám vybraný model.
Serverová Gemma je odvozená varianta Heretic a neběží na notebooku. Programy na
pozadí zůstávají otevřené, případný souběžný přenos je zaznamenaný. PNG zachovává
pixely původního BMP; krátká režie čtení obrazu je měřená. Úspěšné Present není
optické měření monitoru. Archiv pro opakování neobsahuje klíče, váhy, proprietární
DLL ani měřený spustitelný soubor. Herní obsah si zachovává původní práva.

Tools in `tools/`: `build_context75_dataset.py`, `render_context75_videos.py`,
`run_context75.py`, `analyze_context75.py`, `pending_context75_review.py`,
`record_context75_review.py`, `diagnose_context75.py`, `validate_context75.py`,
`publish_context75_data.py`. See each command's `--help`. Always use a new output
directory for a changed policy or a repeat. The original failed result remains
available; do not replace it with a successful retry.

Display deviation / Odchylka zobrazení: run 005 started fullscreen but its last
two proofs show a 1462 × 853 settings window. The reason is not established.
The other 898 proofs are 1920 × 1080. All results remain in the primary analysis;
excluding this run changes Luna-on accuracy from 72.0% to 72.1%. See the public
`display-audit.json` for latency and performance sensitivity.

Běh 005 začal ve fullscreenu, poslední dva snímky ale zachycují okno 1462 × 853.
Příčina přepnutí není doložená. Zbylých 898 snímků má 1920 × 1080. Všechny výsledky
zůstávají započítané; bez tohoto běhu má Luna s pamětí 72,1 % místo 72,0 %.
Rozbor prodlev a výkonu je v `display-audit.json` na webu.
