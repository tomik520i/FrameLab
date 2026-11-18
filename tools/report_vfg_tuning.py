"""Collect the reproducible VFG experiments, without mixing timing and visual export."""
import csv
import html
import json
import pathlib
import shutil
import statistics
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SWEEP = ROOT / 'analysis-output/vfg-sweep-2026-11-18'
REPORT = ROOT / 'analysis-output/vfg-tuning-report-2026-11-18'
DATA = ROOT / 'docs/performance/vfg-tuning-2026-11-18'
sys.path.insert(0, str(ROOT / 'deps/python'))
from PIL import Image, ImageDraw, ImageFont


def rows(path):
    with path.open() as f:
        return list(csv.DictReader(f))


def main():
    DATA.mkdir(parents=True, exist_ok=True)
    offline = []
    for file in sorted(SWEEP.glob('*/summary.json')):
        if file.parent.name.startswith('ps5-sequence'):
            continue
        r = json.loads(file.read_text())
        r['run'] = file.parent.name
        if not r['truth_frames']:
            for k in ('psnr_mean_db', 'hold_psnr_mean_db', 'blend_psnr_mean_db'):
                r[k] = None
        offline.append(r)
        dest = DATA / r['run']
        dest.mkdir(exist_ok=True)
        for name in ('frames.csv', 'timing.csv'):
            shutil.copy2(file.parent / name, dest / name)
        (dest / 'summary.json').write_text(json.dumps(r, indent=2), encoding='utf-8')
    for file in SWEEP.glob('*.csv'):
        shutil.copy2(file, DATA / file.name)
    identical = []
    for dataset in ('csgo', 'ps5'):
        for mult in (3, 4):
            a = rows(SWEEP / f'{dataset}-{mult}x-q1-cut1-legacy/frames.csv')
            b = rows(SWEEP / f'{dataset}-{mult}x-q1-cut1-fixed/frames.csv')
            assert [(x['pair_id'], x['index']) for x in a] == [(x['pair_id'], x['index']) for x in b]
            identical.append({'dataset': dataset, 'multiplier': mult, 'compared': len(a),
                              'rgb_identical': sum(x['rgb_fnv64'] == y['rgb_fnv64'] for x, y in zip(a, b))})
    # Repeat 1 overlapped a CPU PNG export. Keep it in local diagnostics, exclude
    # it from the published speed aggregate rather than conceal the confound.
    selections = [('replay-redraw-2026-11-18', {0}), ('replay-redraw-repeat-2026-11-18', {0, 2}),
                  ('replay-redraw-low-2026-11-18', {0})]
    replay = []
    for folder, repeats in selections:
        for file in sorted((ROOT / 'analysis-output' / folder).glob('*/summary.json')):
            if int(file.parent.name.rsplit('r', 1)[1]) not in repeats:
                continue
            r = json.loads(file.read_text())
            r['experiment'] = folder
            r['args'] = [x.replace(str(ROOT), '<workspace>') for x in r['args']]
            replay.append(r)
            dest = DATA / 'replay' / folder / file.parent.name
            dest.mkdir(parents=True, exist_ok=True)
            for telemetry in file.parent.glob('*.csv'):
                shutil.copy2(telemetry, dest / telemetry.name)
            (dest / 'summary.json').write_text(json.dumps(r, indent=2), encoding='utf-8')
    aggregates = []
    for mult in (3, 4):
        for quality in ('medium', 'low'):
            for legacy in (True, False):
                group = [r for r in replay if r['multiplier'] == mult and r['quality'] == quality and r['legacy_output_wait'] == legacy]
                means = {k: statistics.mean(r['means'][k] for r in group) for k in group[0]['means']}
                ranges = {k: [min(r['means'][k] for r in group), max(r['means'][k] for r in group)] for k in means}
                aggregates.append({'multiplier': mult, 'quality': quality, 'legacy_output_wait': legacy,
                                   'runs': len(group), 'means': means, 'run_mean_ranges': ranges})
    validations = []
    for folder in ('replay-final-2x-check', 'replay-vsync-repeat'):
        for file in sorted((ROOT / 'analysis-output' / folder).glob('*/summary.json')):
            r = json.loads(file.read_text())
            r['experiment'] = folder
            r['args'] = [x.replace(str(ROOT), '<workspace>') for x in r['args']]
            validations.append(r)
            dest = DATA / 'validation' / folder / file.parent.name
            dest.mkdir(parents=True, exist_ok=True)
            for f in file.parent.glob('*.csv'):
                shutil.copy2(f, dest / f.name)
            (dest / 'summary.json').write_text(json.dumps(r, indent=2), encoding='utf-8')
    summary = {'date': '2026-11-18', 'test_system': '../2026-11-18/test-system.json',
               'offline_runs': offline, 'binding_pixel_identity': identical, 'replay': aggregates, 'validation_replays': validations,
               'excluded': 'replay-redraw-repeat-2026-11-18 repeat 1 overlaps CPU PNG export; excluded from performance aggregates.',
               'notes': ['No screen capture. Original recorded R pixels replayed in RAM with original arrival spacing.',
                         'Three clean medium runs per condition; one low run per condition. Normal desktop/Avast remained active.',
                         'FPS describes submitted/OS-observed image IDs, not physical panel scanout or distinct pixels.',
                         'Latency is callback to Present, excluding HDMI source/card/monitor. Per-run means are averaged equally.',
                         'Offline GPU timing excludes readback/export; process timings and replay latency are different measures.',
                         '1080p CS:GO 60fps held-out test: 3x uses 20fps input, 4x uses 15fps input. Compare quality settings within a multiplier.',
                         'PS5 has no correctly timed ground-truth intermediate. Its quality is assessed by paired images, not invented PSNR.',
                         'Output-wait A/B uses the same candidate binary and the same corrected SDK binding; only the scheduler changes.']}
    for target in (DATA / 'summary.json', REPORT / 'measurements.json'):
        target.write_text(json.dumps(summary, indent=2), encoding='utf-8')

    # A held-out real frame alongside all three modes, from existing benchmark BMPs.
    nbytes = 1920 * 1080 * 4
    with (ROOT / 'app/samples/csgo-1080p60.bgra').open('rb') as f:
        f.seek(2 * nbytes)
        truth = Image.frombytes('RGB', (1920, 1080), f.read(nbytes), 'raw', 'BGRX')
    sheet = Image.new('RGB', (2560, 740), '#0c1420')
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.truetype('C:/Windows/Fonts/seguisb.ttf', 24)
    pictures = [('Real withheld frame #2', truth)]
    for q, label in enumerate(('Low', 'Medium', 'High')):
        folder = SWEEP / f'csgo-4x-q{q}-cut1-fixed'
        r = next(r for r in rows(folder / 'frames.csv') if r['pair_id'] == '0' and r['index'] == '2')
        pictures.append((f'{label} / PSNR {float(r["psnr_db"]):.2f} dB', Image.open(folder / 'pair-0-phase-2.bmp').convert('RGB')))
    for i, (label, image) in enumerate(pictures):
        sheet.paste(image.crop((640, 220, 1280, 860)), (i * 640, 54))
        draw.text((i * 640 + 12, 12), label, font=font, fill='white')
    draw.text((12, 708), '1080p / original frames 0 and 4 as inputs / t=0.5 / native 640 x 640 crop / cuts ON', font=font, fill='#b6cede')
    sheet.save(REPORT / 'images/csgo-ground-truth.png')

    lines = ['| Mode | Quality | Loop | Runs | Submitted IDs/s | OS-observed IDs/s | App latency ms | GPU % | Present calls/s |',
             '| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |']
    for r in aggregates:
        v = r['means']
        lines.append(f'| {r["multiplier"]}x | {r["quality"]} | {"previous" if r["legacy_output_wait"] else "new"} | {r["runs"]} | '
                     f'{v["content_submissions_fps"]:.1f} | {v["os_observed_content_fps"]:.1f} | {v["callback_to_present_mean_ms"]:.1f} | '
                     f'{v["gpu_total_percent"]:.1f} | {v["present_calls_fps"]:.1f} |')
    (DATA / 'replay-table.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    gt_lines = ['| Held-out test | Quality | Auto cuts | PSNR dB | Pair ready ms | Endpoint copies |',
                '| --- | --- | --- | ---: | ---: | ---: |']
    for r in offline:
        if r['run'].startswith('csgo') and r['binding'] == 'once-per-pair':
            gt_lines.append(f'| {r["multiplier"]}x | {["Low", "Medium", "High"][r["quality"]]} | {r["automatic_cuts"]} | {r["psnr_mean_db"]:.3f} | {r["pair_ready_mean_ms"]:.2f} | {r["endpoint_copies"]}/{r["outputs"]} |')
    (DATA / 'quality-table.md').write_text('\n'.join(gt_lines) + '\n', encoding='utf-8')
    tr = ''.join('<tr>' + ''.join(f'<td>{html.escape(c.strip())}</td>' for c in line.strip('|').split('|')) + '</tr>' for line in lines[2:])
    qt = ''.join('<tr>' + ''.join(f'<td>{html.escape(c.strip())}</td>' for c in line.strip('|').split('|')) + '</tr>' for line in gt_lines[2:])
    page = '''<!doctype html><html lang="cs"><meta charset="utf-8"><title>FrameLab — ladění 3× / 4×</title>
<style>body{background:#0c1420;color:#e4edf5;font:18px/1.55 system-ui;margin:35px auto;padding:0 25px;max-width:1300px}a{color:#54dfbc}img,video{max-width:100%;border:1px solid #33465a}table{border-collapse:collapse;font-size:15px;width:100%}th,td{padding:8px;border-bottom:1px solid #33465a;text-align:left}h1,h2{line-height:1.2}small{color:#a8bdd0}.box{background:#15263a;padding:18px;border-radius:10px}</style>
<h1>FrameLab: skutečné testy 3× a 4×</h1><p>18. 11. 2026 · RTX 5070 Ti · původní nahrávka 84744531 · bez snímání obrazovky</p>
<div class="box"><b>Doporučení:</b> Balanced 3x = Medium; Fast 4x = Low. Automatickou detekci střihu ponechat zapnutou. Low šetří výpočetní čas, ale některé detaily zhorší. High není automaticky lepší. Závažné artefakty 4× zůstávají.</div>
<h2>Stejné záběry, různá kvalita</h2><p>Tři sekundy původního pohybu zpomalené čtyřikrát. Všechny režimy dostaly stejné A/B snímky. Vlevo jsou původní RAW snímky. Není přidána další interpolace při exportu. Vygenerovaná přesná kopie je označena COPY A/B.</p>
<video controls preload="metadata" src="4x-quality-3seconds-4x-slow.mp4"></video>
<p><a href="4x-quality-3seconds-4x-slow.mp4">Stáhnout MP4</a> · <a href="4x-quality-3seconds-4x-slow.csv">ID každého snímku videa</a> · <a href="settings.json">Nastavení a SHA-256 obrázků</a></p>
<p>Video je zarovnané podle času zdroje, proto nezobrazuje odezvu. Používá 67 původních zpracovaných párů; jeho 60fps export není měřením živých FPS aplikace.</p>
<h2>Výřezy v původním rozlišení</h2><p>U nohou postavy je Medium silně zdvojené. Low zde vypadá lépe; ani Low není bez chyb. Kliknutím otevři plný 1:1 výřez.</p>
<a href="images/quality-22768-phase-2.png"><img src="quality-preview.png" alt="Original A, Low, Medium, High, Medium bez detekce střihu a Original B"></a>
<p><a href="images/quality-22768-phase-1.png">t=0,25</a> · <a href="images/quality-22768-phase-2.png">t=0,5</a> · <a href="images/quality-22768-phase-3.png">t=0,75</a></p>
<p>Protiargument k vypnutí detekce střihu: níže Medium a High zvolily přesnou kopii A. Vypnutá detekce vytvoří nový snímek, ale přidá výrazné duchy. Ostrá kopie není důkaz kvalitní interpolace.</p>
<a href="images/quality-22873-phase-2.png"><img src="images/quality-22873-phase-2.png" alt="Occlusion and automatic fallback"></a>
<h2>Odezva celé aplikace při opakování stejného RAW klipu</h2><p>2560×1440, 9,5–12,5 s, původní rozestupy příchodů, opakované 20sekundové běhy. Prvních pět sekund vynecháno. Medium: tři čisté běhy pro každou variantu; Low: jeden běh. Záznam ani export při těchto zahrnutých měřeních neběžely.</p>
<table><thead><tr><th>Mode</th><th>Quality</th><th>Loop</th><th>Runs</th><th>Submitted IDs/s</th><th>OS IDs/s</th><th>App ms</th><th>GPU %</th><th>Present/s</th></tr></thead><tbody>''' + tr + '''</tbody></table>
<p>Nová smyčka obsluhuje GPU bez čekání na překreslení okna a zbytečně nepřekresluje stejný obraz. Srovnání „previous/new“ používá stejný program s přepínačem plánování. Low proti Medium navíc mění samotný model kvality.</p>
<p>FPS jsou ID odeslaná aplikací a pozorovaná Windows, nikoli kamerou ověřené snímky monitoru ani počet rozdílných pixelových obrazů. Odezva začíná přijetím pixelů a končí Present; neobsahuje zdroj, střihovku a monitor. GPU procenta patří celé kartě včetně ostatních aplikací.</p>
<h2>Kontrola proti skutečným snímkům</h2><p>CS:GO 1080p60: skutečné mezisnímky byly vynechány a pak porovnány s dopočtem. Vyšší PSNR znamená menší pixelovou chybu, nikoli bezchybný obraz. Test 3× má vstup 20fps, test 4× vstup 15fps: porovnávej nastavení uvnitř stejného násobiče.</p>
<table><thead><tr><th>Test</th><th>Quality</th><th>Auto cuts</th><th>PSNR dB</th><th>Pair ms</th><th>Copies</th></tr></thead><tbody>''' + qt + '''</tbody></table>
<a href="images/csgo-ground-truth.png"><img src="images/csgo-ground-truth.png" alt="Actual withheld frame and Low Medium High"></a>
<p>U PS5 neexistuje správně časovaný referenční mezisnímek, proto pro ni nevymýšlím PSNR. Oprava vazby vstupů SDK podle oficiální ukázky zachovala všech 430 porovnaných výstupů pixelově totožných; není příčinou vizuálního zlepšení.</p>
<p><a href="measurements.json">Kompletní agregované výsledky a omezení v JSON</a>. Zdrojové CSV, páry, nástroje a popis reprodukce jsou v docs/performance/vfg-tuning-2026-11-18 a docs/vfg-tuning.md.</p></html>'''
    validation_text = '<h2>Kontrola VSync a původního 2×</h2><p>Dva další čisté 20sekundové běhy se zapnutým VSync: 4× Medium ve staré smyčce 90–97 odeslaných ID/s, v nové přibližně 126 ID/s. Odezva klesla z 50–53 na 44 ms. GPU zde vykonává více užitečné práce a průměrné celkové vytížení vzrostlo přibližně z 41 na 51 %. Samostatné 16sekundové ověření 2× bez VSync: 77,8 → 77,7 ID/s, bez praktické změny rychlosti. Výsledky jsou oddělené od hlavní tabulky; úplná CSV jsou v adresáři validation.</p>'
    page = page.replace('<h2>Kontrola proti skutečným snímkům</h2>', validation_text + '<h2>Kontrola proti skutečným snímkům</h2>')
    (REPORT / 'index.html').write_text(page, encoding='utf-8')
    print('\n'.join(lines))
    print('Report:', REPORT / 'index.html')


if __name__ == '__main__':
    main()
