"""Validate and package comparison evidence, preserving the source recording."""
from pathlib import Path
import csv, hashlib, json, shutil, subprocess, sys, zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'deps/python'))
from PIL import Image, ImageDraw, ImageFont
import numpy as np


def sha(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def main():
    take = Path(sys.argv[1]).resolve()
    out = ROOT / 'release-assets' / take.name
    data = json.loads((out / 'comparison-settings.json').read_text())
    examples = ROOT / 'examples' / take.name.removeprefix('analysis-15s-')
    images = out / 'images'
    font = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 20)
    s = next(s for s in data['samples'] if not s['exact_A'] and not s['exact_B'])
    a, g, b = [Image.open(out / name).convert('RGB') for name in s['full_images']]
    box = tuple(s['crops']['character_xyxy'])
    tile_w, tile_h = box[2] - box[0], box[3] - box[1]
    phase = float(s['timestep'])
    blend = Image.blend(a, b, phase)
    sheet = Image.new('RGB', (4 * tile_w, tile_h + 46), '#0b1320')
    d = ImageDraw.Draw(sheet)
    for i, (im, label) in enumerate([(a, 'RAW A'), (blend, f'LINEAR BLEND t={phase:.2f}'), (g, f'ACTUAL VFG t={phase:.2f}'), (b, 'RAW B')]):
        sheet.paste(im.crop(box), (i*tile_w, 46))
        d.text((i*tile_w+12, 8), label, fill='#54dfbc' if i==2 else 'white', font=font)
    sheet.save(images / 'vfg-versus-linear-blend.png')
    aa = np.asarray(g.crop(box)).astype(np.int16)
    bb = np.asarray(blend.crop(box)).astype(np.int16)
    blend_check = {'generated_id': s['generated_id'], 'timestep': phase, 'crop_xyxy': list(box),
                   'exactly_equal_to_rounded_linear_blend': bool(np.array_equal(aa, bb)),
                   'mean_absolute_rgb_difference_0_255': float(np.mean(abs(aa-bb))),
                   'meaning': 'Difference from a crossfade, not an interpolation accuracy or quality score.'}
    (out / 'blend-check.json').write_text(json.dumps(blend_check, indent=2), encoding='utf-8')
    page = (out / 'index.html').read_text(encoding='utf-8')
    addition = f'<h2>Comparison with a simple crossfade</h2><p>RAW A / linear blend at t={phase:.2f} / actual stored VFG / RAW B, identical native-pixel crop. The blend is a labelled reference only; it is not inserted into any movie. Pixel difference is not an accuracy score.</p><img src="images/vfg-versus-linear-blend.png">'
    findings = examples / 'visual-findings.html'
    if findings.exists():
        addition += findings.read_text(encoding='utf-8')
    if 'Comparison with a simple crossfade' not in page:
        page = page.replace('<h2>Full 15-second', addition + '<h2>Full 15-second')
    (out / 'index.html').write_text(page, encoding='utf-8')
    examples.mkdir(parents=True, exist_ok=True)
    for pattern in ('pair-*_full.png', 'pair-*_character.png', 'pair-*_hud.png', 'vfg-versus-linear-blend.png', 'phases-*.png'):
        for path in images.glob(pattern):
            shutil.copy2(path, examples / path.name)
    for name in ('README.md', 'README.cs.md'):
        if (examples / name).exists():
            md = (examples / name).read_text(encoding='utf-8')
            md = md.replace('(pair-', '(images/pair-').replace('(vfg-versus-', '(images/vfg-versus-').replace('(phases-', '(images/phases-')
            md = md.replace(' · [FrameLab](../../README.md)', '').replace(' · [Český návod FrameLabu](../../docs/README.cs.md)', '')
            md = md.replace('(../../docs/', '(https://github.com/tomik520i/FrameLab/blob/main/docs/').replace('(../82183953/', '(https://github.com/tomik520i/FrameLab/blob/main/examples/82183953/')
            (out / name).write_text(md, encoding='utf-8')
    ffmpeg = next((ROOT / 'deps/python/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
    validation = {'videos': [], 'frame_maps': [], 'blend_check': blend_check}
    for path in sorted(out.glob('*.mp4')):
        result = subprocess.run([str(ffmpeg), '-hide_banner', '-v', 'error', '-threads', '2', '-i', str(path),
                                 '-progress', 'pipe:1', '-nostats', '-f', 'null', '-'],
                                capture_output=True, text=True, check=True)
        progress = dict(line.split('=', 1) for line in result.stdout.splitlines() if '=' in line)
        count = int(progress['frame']); duration = int(progress['out_time_us']) / 1e6
        clip = next((c for c in data['clips'] if c['file'] == path.name), None)
        expected_duration = clip['playback_duration_s'] if clip else json.loads((take/'result.json').read_text())['duration_seconds']
        expected_fps = clip['playback_fps'] if clip else 60
        assert count == round(expected_duration*expected_fps) and abs(duration-expected_duration) < .02, (path.name, count, duration)
        assert not result.stderr.strip(), result.stderr
        validation['videos'].append({'file': path.name, 'decoded_frames': count, 'duration_s': duration, 'decode_errors': 0})
        print('Decoded and checked', path.name, flush=True)
    frame_keys = {(r['kind'], int(r['id'])) for r in csv.DictReader((out / 'frames.csv').open())}
    for clip in data['clips']:
        rows = list(csv.DictReader((out / clip['frame_map']).open()))
        assert len(rows) == round(clip['playback_duration_s']*clip['playback_fps'])
        for i, r in enumerate(rows):
            assert int(r['video_frame']) == i
            assert (r['left_kind'], int(r['left_id'])) in frame_keys
            assert (r['right_kind'], int(r['right_id'])) in frame_keys
        validation['frame_maps'].append({'file': clip['frame_map'], 'rows': len(rows), 'all_references_found': True,
                                        'raw_ids_left': len({r['left_id'] for r in rows}),
                                        'raw_ids_right': len({r['right_id'] for r in rows if r['right_kind']=='R'}),
                                        'generated_ids_right': len({r['right_id'] for r in rows if r['right_kind']=='G'})})
    # Decode actual MP4 frames for visual QA rather than inspecting only pre-encode pixels.
    qa = ROOT / 'analysis-output' / take.name
    qa.mkdir(parents=True, exist_ok=True)
    for clip in data['clips']:
        target = qa / (clip['mode']+'-decoded.png')
        subprocess.run([str(ffmpeg), '-hide_banner', '-v', 'error', '-ss', '6.6', '-i', str(out/clip['file']),
                        '-frames:v', '1', '-update', '1', '-y', str(target)], check=True)
    (out / 'validation.json').write_text(json.dumps(validation, indent=2), encoding='utf-8')
    originals = json.loads((take / 'clips.json').read_text())
    assets = []
    for p in sorted(out.rglob('*')):
        if p.is_file() and p.name not in ('manifest.json', 'SHA256SUMS.txt'):
            assets.append({'file': p.relative_to(out).as_posix(), 'bytes': p.stat().st_size, 'sha256': sha(p)})
    manifest = {'recording_id': take.name, 'settings_id': data['settings_id'], 'entry_point': 'index.html',
                'generated_by': ['audit_analysis_take.py','build_comparison_materials.py','finalize_comparison_release.py'],
                'original_lossless_clips_not_bundled': [x for x in originals['clips'] if x['lossless']],
                'clips': data['clips'], 'assets': assets, 'audio_recorded': False,
                'rights': 'Game footage remains under its respective rights holders; source GPL does not license game assets.'}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    (out / 'SHA256SUMS.txt').write_text('\n'.join(f'{a["sha256"]}  {a["file"]}' for a in assets) +
                                      f'\n{sha(out/"manifest.json")}  manifest.json\n', encoding='utf-8')
    archive = ROOT / 'dist' / f'FrameLab-{take.name.removeprefix("analysis-15s-")}-comparison-example.zip'
    if archive.exists():
        raise FileExistsError(f'Preserving existing archive: {archive.name}')
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=4) as z:
        for p in sorted(out.rglob('*')):
            if p.is_file():
                z.write(p, p.relative_to(out))
    print(json.dumps({'archive': archive.name, 'bytes': archive.stat().st_size, 'sha256': sha(archive),
                      'validated_videos': validation['videos'], 'frame_maps': validation['frame_maps']}, indent=2), flush=True)


if __name__ == '__main__':
    main()
