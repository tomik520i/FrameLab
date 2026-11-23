"""Render the frozen 23 November integration evidence; never rerun inference."""
from pathlib import Path
import html, json, statistics

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'website/runs/2026-11-23-multilang'

def esc(value):
    return html.escape(str(value), quote=True)

def main():
    translations = [json.loads(line) for line in (OUT/'translation/translations.jsonl').read_text(encoding='utf-8').splitlines()]
    ocr = json.loads((OUT/'ocr-final/ocr-results.json').read_text(encoding='utf-8'))
    assert len(translations) == 56 and len(ocr) == 44
    rows = ''.join(f'<tr><td>{r["id"]}</td><td>{esc(r["source_language"])} → {esc(r["target_language"])}</td><td dir="auto">{esc(r["source"])}</td><td dir="auto">{esc(r["translation"])}</td><td>{r["ms"]:.0f}</td></tr>' for r in translations)
    ocr_rows = ''.join(f'<tr><td><a href="ocr-final/{esc(r["language"])}-{r["size"]}.png">{esc(r["language"])} / {r["size"]}px ↗</a></td><td dir="auto">{esc(r["expected"])}</td><td dir="auto">{esc(r["ocr"])}</td><td>{r["cer"]*100:.2f}%</td></tr>' for r in ocr)
    body = '''<nav><a href="../../?lang=cs">← FrameLab</a><a href="../../guide/multilingual.html">Český návod</a><a href="../../guide/multilingual-en.html">English guide</a><a href="../2026-11-22-rife/">RIFE: obraz a výkon / image and speed</a></nav>
<p class="eyebrow">23 NOV 2026 / PREVIEW.17 / RTX 4050 LAPTOP 6 GB</p>
<h1>Více jazyků. Skutečné výstupy.<br><span lang="en">More languages. Actual outputs.</span></h1>
<p>Integrační zkouška TranslateGemma 4B Q4_K_M: <b>56 krátkých překladů, 16 směrů, 10 jazyků</b>. Všechny výsledky jsou níže včetně chyb. Není to zaslepené hodnocení rodilými mluvčími ani srovnání kvality s CUNI. Čtyřicet voleb v aplikaci neznamená ověření všech 1 560 jazykových párů.</p>
<p lang="en">An integration smoke test: <b>56 short translations, 16 directions, 10 languages</b>. Every output is retained below, including errors. This is neither a blinded native-speaker assessment nor a CUNI quality comparison. Forty UI choices do not verify all 1,560 pairs.</p>
<div class="cards"><p><strong>431 ms</strong>medián překladu / median translation</p><p><strong>566 ms</strong>P95 překladu / translation P95</p><p><strong>2.56 GiB</strong>GPU residency reported by runtime</p></div>
<p>Časy nezahrnují OCR ani načtení modelu. Zahřáté načtení trvalo 16,3 s, první studený start přibližně 59,4 s. Všech 35 vrstev běželo na GPU. Q8 a 12B nebyly na tomto notebooku otestovány.</p>
<p lang="en">Times exclude OCR and model loading. Warm loading took 16.3 s; the first cold start took about 59.4 s. All 35 layers ran on the GPU. Q8 and 12B were not tested on this laptop.</p>
<h2>Kvalita a současný běh / Quality and concurrent use</h2>
<p>Q4 v češtině udělalo zjevné chyby: „Nevyotevívejte červenou dveř.“ a „Most už není. Potřebujeme najít jinou cestu přes.“ Přidalo „jen“ a jinde vynechalo „for you“. CUNI zůstává samostatnou volbou pro EN → CS; netvrdíme, že Gemma překládá lépe.</p>
<p lang="en">Q4 produced clear Czech grammar and meaning errors: malformed words, an unfinished phrase, an added “only” and an omitted “for you”. CUNI remains an EN → CS option; these tests do not establish Gemma as better.</p>
<p>Nativní OCR → překlad prošel DE → EN, přepnutím stejného vstupu DE → CS a JA → EN, včetně vymazání starého překladu a uvolnění modelu. RIFE Low + NR 360p + Gemma běžely současně kolem 58–60 odeslaných snímků/s; titulek byl konstantní a překlad uložený v cache. <b>Není to test průběžně se měnícího dialogu.</b> Celé GPU tehdy hlásilo přibližně 3 702 z 6 141 MiB a 86 % využití; nejde o samotnou spotřebu modelu.</p>
<p lang="en">The native pipeline passed DE → EN, an unchanged-input switch to DE → CS, and JA → EN, with stale-caption flushing and model unloading. RIFE Low + NR 360p + Gemma ran together at roughly 58–60 submitted frames/s with a constant cached caption. <b>This does not benchmark continuously changing dialogue.</b> Whole-GPU telemetry was about 3,702/6,141 MiB and 86% utilization, not model-only usage.</p>
<h2>Všech 56 překladů / All 56 translations</h2><p><a href="translation/translations.jsonl">Original JSONL</a> · <a href="translation/meta.json">Runtime metadata</a></p>
<div class="table"><table><thead><tr><th>ID</th><th>Pair</th><th>Input</th><th>Actual output</th><th>ms</th></tr></thead><tbody>TRANSLATION_ROWS</tbody></table></div>
<h2>OCR a vykreslení / OCR and rendering</h2>
<p>44 syntetických obrázků, 22 jazyků, dvě velikosti písma. 23 přesných shod; průměr CER počítaný za každý obrázek 4,47 %. Žádný chybějící vykreslený znak. Všech 12 OCR rodin se načetlo. Arabština, vietnamština a rozdělené turecké řádky ukazují problémy. Toto <b>není přesnost OCR na hrách</b>.</p>
<p lang="en">44 synthetic images, 22 languages, two font sizes. 23 exact matches; mean per-image character error rate (CER) 4.47%. No missing rendered glyphs. All 12 OCR families initialized. Arabic, Vietnamese and split Turkish lines expose weaknesses. This is <b>not game OCR accuracy</b>.</p>
<p><a href="ocr-final/ocr-results.json">Original OCR JSON</a> · Odkaz v prvním sloupci otevře přesný vstup / first-column links open exact inputs.</p>
<div class="table"><table><thead><tr><th>Fixture</th><th>Expected</th><th>OCR output</th><th>CER</th></tr></thead><tbody>OCR_ROWS</tbody></table></div>
<h2>Rozsah kontroly / Validation scope</h2>
<p>Jazykové protokoly, obnova po načtení, vykreslení a hardwarové blokace mají samostatné testy. Lokální HTTP regresní test stahování/pokračování v tomto prostředí opakovaně narazil na přerušení spojení WinError 10054. Skutečné HTTPS stažení 2,49GB modelu a ověření SHA-256 prošlo, ale nenahrazuje chybějící výsledek tohoto regresního testu.</p>
<p lang="en">Language protocol, loading recovery, rendering and hardware guards have separate checks. The local HTTP download/resume regression encountered intermittent WinError 10054 connection resets in this environment. The real 2.49 GB HTTPS model download and SHA-256 verification passed; this does not replace the missing regression result.</p>
<p><a href="https://github.com/tomik520i/FrameLab/blob/v0.2.0-preview.17/docs/release-preview17.md">Release notes and methodology</a> · <a href="../../licenses/">Licenses</a></p>'''.replace('TRANSLATION_ROWS', rows).replace('OCR_ROWS', ocr_rows)
    css = '*{box-sizing:border-box}body{margin:0;background:#101311;color:#f2f3e8;font:17px/1.65 system-ui,sans-serif}main{max-width:1250px;margin:auto;padding:32px clamp(18px,4vw,55px) 80px}a{color:#bdff57;text-underline-offset:4px}nav{display:flex;flex-wrap:wrap;gap:20px;border-bottom:1px solid #34402f;padding-bottom:20px}h1{font-size:clamp(30px,4vw,56px);line-height:1.15}h1 span{color:#a5af9e}h2{margin-top:55px;font-size:28px}.eyebrow{color:#ff8256;font:13px monospace;letter-spacing:2px}p[lang=en]{color:#a5af9e}.cards{display:grid;grid-template-columns:repeat(3,1fr);gap:20px}.cards p{padding:20px;background:#1a2117}.cards strong{display:block;color:#bdff57;font-size:36px}.table{overflow-x:auto}table{border-collapse:collapse;min-width:720px;width:100%;font-size:15px}th,td{text-align:start;vertical-align:top;padding:12px;border-bottom:1px solid #34402f}td:nth-child(2){min-width:100px}tr:hover{background:#1a2117}@media(max-width:650px){.cards{grid-template-columns:1fr;gap:0}.cards p{margin:5px 0}.cards strong{font-size:28px}}'
    (OUT/'index.html').write_text(f'<!doctype html><html lang="cs"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FrameLab — multilingual integration evidence</title><style>{css}</style></head><body><main>{body}</main></body></html>', encoding='utf-8')
    print('Published report from frozen evidence:', len(translations), 'translations;', len(ocr), 'OCR images; median', statistics.median(r['ms'] for r in translations))

if __name__ == '__main__':
    main()
