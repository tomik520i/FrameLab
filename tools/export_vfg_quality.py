"""Pixel-preserving visual evidence for the recorded-input VFG quality sweep."""
import argparse
import bisect
import csv
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'deps/python'))
from PIL import Image, ImageDraw, ImageFont

W, H = 2560, 1440
BYTES = W * H * 4
CROP = (960, 400, 1600, 1040)
FONT = ImageFont.truetype('C:/Windows/Fonts/seguisb.ttf', 25)
SMALL = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 20)


def csv_rows(path):
    with path.open() as f:
        return list(csv.DictReader(f))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sweep', required=True, type=pathlib.Path)
    parser.add_argument('--out', required=True, type=pathlib.Path)
    parser.add_argument('--stills-only', action='store_true')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    (out / 'images').mkdir(exist_ok=True)
    recording = ROOT / 'app/recordings/analysis-15s-84744531'
    raw_rows = [r for r in csv_rows(recording / 'frames.csv') if r['kind'] == 'R']
    raw_by_id = {int(r['id']): r for r in raw_rows}
    raw_file = (recording / 'raw.bgra').open('rb')
    settings = [(0, 1, 'Low'), (1, 1, 'Medium'), (2, 1, 'High'), (1, 0, 'Medium - cuts off')]
    runs = []
    for quality, cuts, label in settings:
        folder = args.sweep / f'ps5-sequence-4x-q{quality}-cut{cuts}-fixed'
        rows = csv_rows(folder / 'frames.csv')
        runs.append({'label': label, 'file': (folder / 'generated.bgra').open('rb'), 'folder': folder,
                     'rows': rows, 'by_id': {(int(r['pair_id']), int(r['index'])): r for r in rows},
                     'summary': json.loads((folder / 'summary.json').read_text())})

    def pixels(file, offset):
        file.seek(int(offset))
        data = file.read(BYTES)
        if len(data) != BYTES:
            raise RuntimeError('Truncated evidence pixels')
        return data

    def picture(file, offset):
        return Image.frombytes('RGB', (W, H), pixels(file, offset), 'raw', 'BGRX')

    manifest = {'recording_id': recording.name, 'dimensions': [W, H], 'crop_xyxy': CROP,
                'backend': 'NVIDIA VFG', 'multiplier': 4, 'images': [], 'runs': [r['summary'] for r in runs],
                'limits': 'Same stored source A/B pairs in every quality run. No PS5 intermediate ground truth. '
                          'Source-aligned, so processing latency is deliberately absent from the video; see real-time replay telemetry. '
                          'Generated pixels are not recomputed, blended or interpolated during export.'}
    for pair in (22768, 22873):
        for phase in (1, 2, 3):
            b_id = int(runs[0]['by_id'][pair, phase]['source_B'])
            items = [('Original A', raw_file, raw_by_id[pair]['byte_offset'])]
            for run in runs:
                row = run['by_id'][pair, phase]
                copy = ' [COPY A]' if row['exact_A'] == '1' else ' [COPY B]' if row['exact_B'] == '1' else ''
                items.append((run['label'] + copy, run['file'], row['byte_offset']))
            items += [('Original B', raw_file, raw_by_id[b_id]['byte_offset'])]
            sheet = Image.new('RGB', (640 * len(items), 750), '#0c1420')
            d = ImageDraw.Draw(sheet)
            for i, (label, file, offset) in enumerate(items):
                image = picture(file, offset)
                name = f'pair-{pair}-t{phase}of4-{i}.png'
                image.save(out / 'images' / name)
                sheet.paste(image.crop(CROP), (i * 640, 58))
                d.text((i * 640 + 12, 14), label, font=FONT, fill='white')
                manifest['images'].append({'file': 'images/' + name, 'pair_A': pair, 'pair_B': b_id,
                    'phase': phase / 4, 'label': label, 'offset': int(offset),
                    'png_sha256': hashlib.sha256((out / 'images' / name).read_bytes()).hexdigest(),
                    'full_rgb_sha256': hashlib.sha256(image.tobytes()).hexdigest()})
            d.text((12, 714), f'A #{pair} / B #{b_id} | t={phase/4:.2f} | native 640 x 640 crops | same inputs in every mode', font=SMALL, fill='#abc4dc')
            sheet.save(out / 'images' / f'quality-{pair}-phase-{phase}.png')
            if pair == 22768 and phase == 2:
                sheet.resize((1920, 375), Image.Resampling.LANCZOS).save(out / 'quality-preview.png')
    (out / 'settings.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print('Saved six native-pixel comparison sheets and full frames', flush=True)
    if args.stills_only:
        return

    # Build a timeline containing only recorded originals and outputs from the GPU runs.
    events = [(float(r['time_s']), 'R', int(r['id']), 0, int(r['byte_offset'])) for r in raw_rows]
    raw_times = [e[0] for e in events]
    generated_events = []
    for run in runs[:3]:
        es = []
        for row in run['rows']:
            a, b, phase = int(row['source_A']), int(row['source_B']), int(row['index'])
            ta, tb = float(raw_by_id[a]['time_s']), float(raw_by_id[b]['time_s'])
            if phase == 1:
                es.append((ta, 'R', a, 0, int(raw_by_id[a]['byte_offset'])))
            es.append((ta + (tb - ta) * phase / 4, 'G', a, phase, int(row['byte_offset'])))
            if phase == 3:
                es.append((tb, 'R', b, 0, int(raw_by_id[b]['byte_offset'])))
        es.sort()
        generated_events.append((es, [e[0] for e in es]))
    ffmpeg = next((ROOT / 'deps/python/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
    video = out / '4x-quality-3seconds-4x-slow.mp4'
    args_ff = [str(ffmpeg), '-v', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '1920x600',
               '-r', '60', '-i', 'pipe:0', '-an', '-c:v', 'libx264', '-preset', 'medium', '-crf', '17',
               '-threads', '4', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', '-y', str(video)]
    encoder = subprocess.Popen(args_ff, stdin=subprocess.PIPE, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    cache = [{} for _ in range(4)]
    canvas = Image.new('RGB', (1920, 600), '#0c1420')
    d = ImageDraw.Draw(canvas)
    labels = ['ORIGINAL / RAW', '4x - LOW', '4x - MEDIUM', '4x - HIGH']
    for col, label in enumerate(labels):
        d.text((col * 480 + 12, 6), label, font=FONT, fill='white')
    mapping = []
    try:
        for frame in range(720):
            time = 9.5 + frame / 240
            r = events[max(0, bisect.bisect_right(raw_times, time) - 1)]
            row = {'video_frame': frame, 'video_time_s': frame / 60, 'source_clock_s': time, 'raw_id': r[2]}
            chosen = [(r, raw_file)]
            for index, (es, times) in enumerate(generated_events):
                k = bisect.bisect_right(times, time) - 1
                e = es[k] if k >= 0 else r
                chosen.append((e, runs[index]['file'] if e[1] == 'G' else raw_file))
                row[f'q{index}_kind'] = e[1]
                row[f'q{index}_id_or_pair_A'] = e[2]
                row[f'q{index}_phase_index'] = e[3]
            for col, (e, file) in enumerate(chosen):
                key = (e[1], e[2], e[3])
                if key not in cache[col]:
                    cache[col].clear()
                    cache[col][key] = picture(file, e[4]).crop(CROP).resize((480, 480), Image.Resampling.LANCZOS)
                canvas.paste(cache[col][key], (col * 480, 60))
                caption = f'Original #{e[2]}'
                if e[1] == 'G':
                    row_info = runs[col - 1]['by_id'][e[2], e[3]]
                    copy = ' / COPY A' if row_info['exact_A'] == '1' else ' / COPY B' if row_info['exact_B'] == '1' else ''
                    caption = f'A #{e[2]} / t={e[3]/4:.2f}{copy}'
                d.rectangle((col * 480, 36, col * 480 + 479, 59), fill='#0c1420')
                d.text((col * 480 + 12, 34), caption, font=SMALL, fill='#b5cee2')
            d.rectangle((0, 540, 1920, 600), fill='#0c1420')
            d.text((12, 548), f'4x slow motion | source {time:.3f} s | same A/B pairs, cuts ON | source-aligned, latency excluded', font=SMALL, fill='#b5cee2')
            d.text((12, 575), '1440p input | fixed crop scaled 640 -> 480 px | comparison export is 60 fps; it does not measure live output FPS', font=SMALL, fill='#b5cee2')
            encoder.stdin.write(canvas.tobytes())
            mapping.append(row)
    finally:
        encoder.stdin.close()
    if encoder.wait() != 0:
        raise RuntimeError('Video encoder failed')
    with video.with_suffix('.csv').open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=list(mapping[0]))
        writer.writeheader()
        writer.writerows(mapping)
    manifest['video'] = {'file': video.name, 'sha256': hashlib.sha256(video.read_bytes()).hexdigest(),
                         'source_start_s': 9.5, 'source_end_s': 12.5, 'duration_s': 12, 'fps': 60,
                         'frames': 720, 'slowdown': 4, 'source_pairs': len(runs[0]['rows']) // 3}
    (out / 'settings.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print('Saved 12-second comparison video + per-frame mapping + settings JSON', flush=True)


if __name__ == '__main__':
    main()
