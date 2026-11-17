"""Build reproducible visual evidence from recorded pixels, never synthesize new frames.

Run audit_analysis_take.py first. The source-aligned movie removes scheduling delay;
the submission movie preserves recorded presentation timing and latency.
"""
from pathlib import Path
import argparse, bisect, csv, functools, hashlib, html, json, shutil, subprocess, sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'deps/python'))
import numpy as np
from PIL import Image, ImageDraw, ImageFont


def digest(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('take', type=Path)
    parser.add_argument('--start', type=float, default=3.0)
    parser.add_argument('--seconds', type=float, default=3.0)
    parser.add_argument('--slowdown', type=float, default=4.0)
    parser.add_argument('--sample-id', type=int, action='append', help='Generated IDs selected after visual inspection; repeat for each example.')
    parser.add_argument('--selection-reason', default='Continuous visible traversal / swing; includes camera and foreground motion.')
    args = parser.parse_args()
    take = args.take.resolve()
    audit_dir = ROOT / 'analysis-output' / take.name
    audit = json.loads((audit_dir / 'audit.json').read_text())
    settings = json.loads((take / 'session.json').read_text())
    settings.update(json.loads((take / 'capture-settings.json').read_text()))
    multiplier = int(settings['multiplier'])
    backend = settings['backend']
    if args.start < .1 or args.seconds <= 0 or args.slowdown < 1 or args.start + args.seconds > audit['result']['duration_seconds']:
        raise ValueError('Choose a positive segment inside the complete recording, after the startup boundary.')
    out = ROOT / 'release-assets' / take.name
    out.mkdir(parents=True, exist_ok=True)
    images = out / 'images'
    images.mkdir(exist_ok=True)
    rows = list(csv.DictReader((take / 'frames.csv').open()))
    for r in rows:
        for k in ('id', 'source_A', 'source_B', 'width', 'height', 'byte_offset', 'byte_count'):
            r[k] = int(r[k])
        for k in ('time_s', 'timestep'):
            r[k] = float(r[k])
    lookup = {(r['kind'], r['id']): r for r in rows}
    raw = sorted((r for r in rows if r['kind'] == 'R'), key=lambda r: r['time_s'])
    generated = sorted((r for r in rows if r['kind'] == 'G'), key=lambda r: r['time_s'])
    presents = list(csv.DictReader((take / 'presents.csv').open()))
    for p in presents:
        p['present_time_s'] = float(p['present_time_s'])
        p['image_id'] = int(p['image_id'])
    maps = {k: np.memmap(take / f'{name}.bgra', dtype=np.uint8, mode='r')
            for k, name in [('R', 'raw'), ('G', 'generated')]}

    def pixels(key):
        r = lookup[key]
        return maps[key[0]][r['byte_offset']:r['byte_offset'] + r['byte_count']].reshape(r['height'], r['width'], 4)

    def full_image(key):
        return Image.fromarray(pixels(key)[:, :, 2::-1].copy())

    @functools.lru_cache(maxsize=16)
    def preview(key):
        return full_image(key).resize((960, 540), Image.Resampling.LANCZOS)

    # Hash full RGB pixels. Alpha/X bytes are deliberately excluded.
    hashes = {}
    raw_changes = []
    previous = None
    for r in raw:
        key = ('R', r['id'])
        h = hashlib.sha256(pixels(key)[:, :, :3].tobytes()).hexdigest()
        hashes[key] = h
        if h != previous:
            raw_changes.append(r)
        previous = h
    print('Hashed raw frames', flush=True)
    for r in generated:
        hashes['G', r['id']] = hashlib.sha256(pixels(('G', r['id']))[:, :, :3].tobytes()).hexdigest()
    print('Hashed generated frames', flush=True)
    changes = []
    pixel_changes = []
    prior_key = prior_hash = None
    for p in presents:
        key = (p['kind'].upper(), p['image_id'])
        if key != prior_key:
            changes.append(p)
        h = hashes.get(key)
        if h is not None and h != prior_hash:
            pixel_changes.append(p)
        prior_key, prior_hash = key, h

    ideal = [(r['time_s'], ('R', r['id'])) for r in raw_changes]
    for r in generated:
        a, b = lookup.get(('R', r['source_A'])), lookup.get(('R', r['source_B']))
        if a and b:
            ideal.append((a['time_s'] + (b['time_s'] - a['time_s']) * r['timestep'], ('G', r['id'])))
    ideal.sort()
    raw_t = [r['time_s'] for r in raw]
    ideal_t = [t for t, _ in ideal]
    present_t = [p['present_time_s'] for p in presents]
    font = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 25)
    small = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 20)
    ffmpeg = next((ROOT / 'deps/python/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
    duration = args.seconds * args.slowdown
    clip_meta = []
    for mode in ('source-aligned', 'recorded-submissions'):
        name = f'{take.name}_{mode}_{args.seconds:g}s_{args.slowdown:g}x-slow.mp4'
        path = out / name
        command = [str(ffmpeg), '-hide_banner', '-loglevel', 'error', '-f', 'rawvideo',
                   '-pixel_format', 'rgb24', '-video_size', '1920x660', '-framerate', '60',
                   '-i', 'pipe:0', '-an', '-c:v', 'libx264', '-threads', '2', '-preset', 'fast',
                   '-crf', '16', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', '-y', str(path)]
        frame_map = []
        with subprocess.Popen(command, stdin=subprocess.PIPE) as process:
            try:
                for n in range(round(duration * 60)):
                    t = args.start + n / (60 * args.slowdown)
                    left = ('R', raw[bisect.bisect_right(raw_t, t) - 1]['id'])
                    if mode == 'source-aligned':
                        right = ideal[bisect.bisect_right(ideal_t, t) - 1][1]
                    else:
                        p = presents[bisect.bisect_right(present_t, t) - 1]
                        right = (p['kind'].upper(), p['image_id'])
                    canvas = Image.new('RGB', (1920, 660), '#0b1320')
                    canvas.paste(preview(left), (0, 80))
                    canvas.paste(preview(right), (960, 80))
                    d = ImageDraw.Draw(canvas)
                    d.text((18, 6), f'RAW input | {len(raw_changes)/audit["result"]["duration_seconds"]:.1f} image changes/s', font=font, fill='white')
                    d.text((978, 6), f'{backend} {multiplier}x | {mode}', font=font, fill='#54dfbc')
                    d.text((18, 43), f'RAW #{left[1]}   |   source clock {t:.3f}s', font=small, fill='white')
                    label = 'GENERATED' if right[0] == 'G' else 'RAW'
                    d.text((978, 43), f'{label} #{right[1]}   |   {args.slowdown:g}x slow motion', font=small, fill='#54dfbc')
                    note = ('Matched source time; scheduling delay removed. Reconstructed cadence, not monitor scanout.'
                            if mode == 'source-aligned' else
                            'Same capture clock; output retains its actual submission delay. Not physical monitor scanout.')
                    d.text((18, 626), note, font=small, fill='#c7d0df')
                    process.stdin.write(canvas.tobytes())
                    frame_map.append({'video_frame': n, 'playback_time_s': n/60, 'capture_clock_s': t,
                                      'left_kind': left[0], 'left_id': left[1], 'right_kind': right[0], 'right_id': right[1]})
                process.stdin.close()
                if process.wait() != 0:
                    raise RuntimeError('Video encoding failed')
            except BaseException:
                process.kill()
                raise
        map_name = path.with_suffix('.csv').name
        with (out / map_name).open('w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=frame_map[0].keys())
            writer.writeheader(); writer.writerows(frame_map)
        clip_meta.append({'clip_id': path.stem, 'file': name, 'settings_id': settings['settings_id'],
                          'recording_id': take.name, 'mode': mode, 'start_s': args.start,
                          'source_duration_s': args.seconds, 'slowdown': args.slowdown,
                          'playback_duration_s': duration, 'playback_fps': 60, 'frame_map': map_name,
                          'width': 1920, 'height': 660, 'audio': False, 'encoding': 'H.264 CRF16 yuv420p',
                          'transform': 'Equal Lanczos resize of actual stored RGB pixels; text outside images; no new interpolation.'})
        print('Encoded', name, flush=True)

    metrics = list(csv.DictReader((audit_dir / 'all-generated-frames.csv').open()))
    selected = json.loads((audit_dir / 'selected-pairs.json').read_text())
    # Curated IDs are explicit, so failures and all phases of one A/B pair can be reproduced.
    if args.sample_id:
        by_id = {int(m['id']): m for m in metrics}
        chosen = [by_id[gid] for gid in args.sample_id]
    else:
        chosen = [min(metrics, key=lambda m: abs(float(m['time_s']) - target))
                  for target in (3.7, 4.65, 6.22, 11.63)]
    samples = []
    for m in chosen:
        gid = int(m['id']); keys = [('R', int(m['A'])), ('G', gid), ('R', int(m['B']))]
        ims = [full_image(key) for key in keys]
        for key, im in zip(keys, ims):
            im.save(images / f'pair-{gid}_{key[0]}-{key[1]}.png')
        for region, box in [('full', None), ('character', (960, 400, 1600, 1040)), ('hud', (1920, 1040, 2560, 1440))]:
            h = 360 if box is None else box[3] - box[1]
            strip = Image.new('RGB', (1920, h + 46), '#0b1320')
            d = ImageDraw.Draw(strip)
            for i, (key, im) in enumerate(zip(keys, ims)):
                tile = im.resize((640, 360), Image.Resampling.LANCZOS) if box is None else im.crop(box)
                strip.paste(tile, (640*i, 46))
                title = ('RAW A', 'GENERATED', 'RAW B')[i]
                phase = f' t={lookup[key]["timestep"]:.2f}' if key[0] == 'G' else ''
                d.text((640*i+12, 8), f'{title} #{key[1]}{phase} | {float(m["time_s"]):.3f}s', font=small, fill='#54dfbc' if i==1 else 'white')
            strip.save(images / f'pair-{gid}_{region}.png')
        samples.append({'generated_id': gid, 'source_A': int(m['A']), 'source_B': int(m['B']),
                        'available_time_s': float(m['time_s']), 'timestep': lookup['G', gid]['timestep'],
                        'exact_A': m['exact_A'] == 'True', 'exact_B': m['exact_B'] == 'True',
                        'full_images': [f'images/pair-{gid}_{key[0]}-{key[1]}.png' for key in keys],
                        'crops': {'character_xyxy': [960,400,1600,1040], 'hud_xyxy': [1920,1040,2560,1440]}})
    for name in ('source-timeline.jpg', 'cadence.png'):
        shutil.copy2(audit_dir / name, images / name)
    screenshot = ROOT / 'docs/images/framelab-live-ui.png'
    if screenshot.exists():
        shutil.copy2(screenshot, images / screenshot.name)
    for name in ('session.json','capture-settings.json','result.json','frames.csv','presents.csv','clips.json'):
        shutil.copy2(take / name, out / name)
    if (take / 'display-confirmations.csv').exists():
        shutil.copy2(take / 'display-confirmations.csv', out / 'display-confirmations.csv')
    for name in ('audit.json', 'all-generated-frames.csv', 'selected-pairs.json'):
        shutil.copy2(audit_dir / name, out / name)
    for suffix in ('comparison', 'output-preview'):
        shutil.copy2(take / f'{take.name}_{suffix}.mp4', out / f'{take.name}_{suffix}.mp4')
    duration_source = audit['result']['duration_seconds']
    comparable = audit['fully_comparable_generated']
    copies = len(audit['exact_endpoint_copies'])
    new = comparable - copies
    selected_window = {'start_s': args.start, 'end_s': args.start+args.seconds,
                       'selection_reason': args.selection_reason,
                       'generated_frames_by_available_time': sum(args.start<=r['time_s']<args.start+args.seconds for r in generated),
                       'exact_endpoint_copies_by_available_time': sum(args.start<=r['time_s']<args.start+args.seconds for r in audit['exact_endpoint_copies'])}
    confirmation_summary = None
    if (take / 'display-confirmations.csv').exists():
        confirmations = list(csv.DictReader((take / 'display-confirmations.csv').open()))
        observed = []
        observed_rgb = 0
        prev_key = prev_hash = None
        for c in confirmations:
            key = (c['kind'].upper(), int(c['image_id']))
            if key != prev_key:
                observed.append(c)
            h = hashes.get(key)
            if h is not None and h != prev_hash:
                observed_rgb += 1
            prev_key, prev_hash = key, h
        confirmation_summary = {
            'events': len(confirmations), 'observed_id_changes': len(observed),
            'observed_ids_per_s': len(observed)/duration_source,
            'generated_id_changes': sum(c['kind'].upper() == 'G' for c in observed),
            'known_rgb_changes': observed_rgb, 'known_rgb_changes_per_s': observed_rgb/duration_source,
            'meaning': 'Sampled Windows DXGI confirmations mapped to image IDs; conservative observation, not physical panel scanout. New IDs may contain copied pixels.'}
    summary = {'recording_id': take.name, 'settings_id': settings['settings_id'], 'settings': settings,
               'capture_fps': len(raw)/duration_source, 'raw_image_changes_per_s': len(raw_changes)/duration_source,
               'submitted_image_ids_per_s': len(changes)/duration_source,
               'known_submitted_rgb_changes': len(pixel_changes),
               'known_submitted_rgb_changes_per_s': len(pixel_changes)/duration_source,
               'fully_comparable_generated': comparable, 'new_vs_both_endpoints': new,
               'exact_endpoint_copies': copies, 'new_vs_endpoints_percent': new/comparable*100,
               'windows_confirmations': confirmation_summary,
               'selected_window': selected_window, 'clips': clip_meta, 'samples': samples,
               'build_provenance': 'pacing_version=2 recorded by app; no commit or executable hash stored at capture time.',
               'limits': audit['limitations'] + ['Source-aligned clip retimes stored images to source A/B plus the recorded timestep; use recorded-submissions clip for measured timing. Captured RAW frames skipped by the generator can also appear in this reconstruction.',
                   'The application screenshot illustrates the UI from the older 2x session; its settings and counters do not describe this take.',
                   'No GPU utilization, GPU execution time or physical monitor cadence time series was stored with this take.',
                   f'{copies} exact endpoint copies are observed; cut-detector fallback is a possible explanation, not a recorded reason.']}
    (out / 'comparison-settings.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
    rows_html = ''.join(f'<tr><th>{html.escape(k)}</th><td>{html.escape(str(v))}</td></tr>' for k,v in settings.items())
    samples_html = ''.join(f'<h3>G #{s["generated_id"]} / A #{s["source_A"]} / B #{s["source_B"]}</h3>'
                          f'<p>{s["available_time_s"]:.3f}s; t={s["timestep"]:.2f}; exact A copy: {s["exact_A"]}</p>'
                          f'<a href="images/pair-{s["generated_id"]}_full.png"><img src="images/pair-{s["generated_id"]}_full.png"></a>'
                          f'<details><summary>Native-pixel character and HUD crops</summary><img src="images/pair-{s["generated_id"]}_character.png"><img src="images/pair-{s["generated_id"]}_hud.png"></details>' for s in samples)
    page = f'''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>FrameLab evidence | {take.name}</title><style>body{{background:#0b1320;color:#dce6f5;font:17px/1.55 system-ui;max-width:1280px;margin:40px auto;padding:0 24px}}h1,h2{{color:#60dfc0}}video,img{{width:100%;height:auto}}a{{color:#75caff}}table{{border-collapse:collapse;width:100%}}td,th{{padding:8px;border-bottom:1px solid #294059;text-align:left}}.card{{background:#132237;padding:20px;border-radius:12px;margin:22px 0}}code{{overflow-wrap:anywhere}}</style>
<h1>FrameLab: {backend} {multiplier}× recording</h1><p>{take.name} · {duration_source:g} seconds · {settings.get('quality_name', settings['quality'])} · {settings['gpu']} · {settings['width']}×{settings['height']} · VSync {settings['vsync']}</p>
<div class="card"><b>More generated images do not guarantee better image quality.</b> {len(raw)} captured frames contain {len(raw_changes)} changing images. {new}/{comparable} comparable generated images differ from both endpoints ({new/comparable*100:.2f}% — not an accuracy score). {copies} are exact endpoint copies. The recorded output has {len(changes)} image-ID changes ({len(changes)/duration_source:.2f}/s), including copies; {len(pixel_changes)} known RGB changes ({len(pixel_changes)/duration_source:.2f}/s). These are submissions, not measured monitor refreshes.</div>
<h2>{args.seconds:g} seconds, slowed to {duration:g} seconds</h2><p>Same {args.start:g}–{args.start+args.seconds:g}s source segment on both sides. {args.slowdown:g}× slow motion exposes held frames, intermediate positions and artifacts. Frame IDs and CSV maps make every displayed image traceable. No interpolation was performed during export.</p>
<h3>Source-aligned comparison</h3><p>Reconstruction using capture timestamps and each recorded A/B timestep. Scheduling delay removed. Captured RAW frames skipped by the generator may appear here too. This isolates image sequence quality; it does not reproduce output timing.</p>
<video controls preload="metadata" src="{clip_meta[0]['file']}"></video>
<h3>Actual recorded submission timeline</h3><p>RAW on the capture clock versus the application's recorded submissions on that same clock. The right side retains processing/queue delay and timing variation. Neither movie records physical screen scanout.</p>
<video controls preload="metadata" src="{clip_meta[1]['file']}"></video>
<h2>Timing across the complete take</h2><p>Image-ID change interval: mean {audit['content_submission_gap_ms']['mean']:.2f} ms, median {audit['content_submission_gap_ms']['median']:.2f} ms, p95 {audit['content_submission_gap_ms']['p95']:.2f} ms, maximum {audit['content_submission_gap_ms']['max']:.2f} ms. Observed G→B ordering anomalies: {len(audit['ordering_anomalies'])}. Recorder drops: {audit['result']['recorder_drops']}. Recorder drops do not count source frames skipped before generation.</p>
<p>RAW capture callback → its output submission: mean {audit['capture_B_to_first_present_ms']['raw']['mean']:.2f} ms, p95 {audit['capture_B_to_first_present_ms']['raw']['p95']:.2f} ms. B callback → generated submission: mean {audit['capture_B_to_first_present_ms']['generated']['mean']:.2f} ms. These clocks exclude source rendering, capture-card buffering and monitor scanout. The latter is measured from B, not the generated midpoint.</p>
<img src="images/cadence.png"><h2>Traceable still images and failure cases</h2>{samples_html}
<h2>Full 15-second A / generated / B preview</h2><video controls preload="metadata" src="{take.name}_comparison.mp4"></video>
<p><a href="{take.name}_output-preview.mp4">Full output preview (60-FPS resampling)</a></p>
<h2>Source timeline</h2><img src="images/source-timeline.jpg">
<h2>Application screenshot</h2><p>User-provided screenshot from the earlier 2× session, Czech UI. Settings/counters in this illustrative image do not describe this take.</p><img src="images/framelab-live-ui.png">
<h2>Settings</h2><table>{rows_html}</table><h2>Data and limitations</h2><p><a href="comparison-settings.json">Comparison IDs/settings</a> · <a href="audit.json">Full audit</a> · <a href="frames.csv">All frame IDs</a> · <a href="presents.csv">All submissions</a> · <a href="all-generated-frames.csv">Per-generated-image metrics</a></p>
<ul>{''.join('<li>'+html.escape(x)+'</li>' for x in summary['limits'])}</ul><p>Game footage belongs to its respective rights holders. FrameLab's GPL license does not license the game assets.</p></html>'''
    (out / 'index.html').write_text(page, encoding='utf-8')
    print(json.dumps({k:v for k,v in summary.items() if k not in ('settings','clips','samples','limits')}, indent=2))
    print(out, flush=True)


if __name__ == '__main__':
    main()
