"""Export actual clean before/after/caption surfaces, preserving their pairing."""
import bisect, hashlib, html, json, math, shutil, subprocess
from pathlib import Path
from PIL import Image, ImageDraw
from recording_archive import Archive

def subtitle_cues(outputs, duration):
    cues = []
    current = None
    for row in outputs:
        text = row.get('subtitle', '')
        source = row.get('subtitle_source', '')
        t = max(0., min(duration, row['time_s']))
        if current and (text, source) == (current['text'], current['source']):
            continue
        if current:
            current['end_s'] = t
            if t > current['start_s']:
                cues.append(current)
        current = {'start_s': t, 'text': text, 'source': source} if text else None
    if current and duration > current['start_s']:
        current['end_s'] = duration
        cues.append(current)
    return cues

def srt_time(seconds):
    ms = max(0, round(seconds * 1000))
    return f'{ms // 3600000:02}:{ms // 60000 % 60:02}:{ms // 1000 % 60:02},{ms % 1000:03}'

def export(folder, ffmpeg, decoder):
    folder = Path(folder)
    session = json.loads((folder / 'session.json').read_text())
    result = json.loads((folder / 'result.json').read_text())
    outputs = [json.loads(line) for line in (folder / 'output-frames.jsonl').read_text(encoding='utf-8').splitlines()]
    if not outputs:
        raise ValueError('No final output frames were recorded; recording is incomplete')
    if any(b['time_s'] < a['time_s'] for a, b in zip(outputs, outputs[1:])):
        raise ValueError('Output timeline is not monotonic')
    archive = Archive(folder, decoder)
    duration = result['duration_seconds']
    w, h, rid = session['width'], session['height'], session['recording_id']
    missing = []
    for row in outputs:
        absent = [stage for stage in ('before', 'after', 'final') if tuple(row[stage]) not in archive.lookup]
        if absent:
            missing.append({'output_id': row['output_id'], 'time_s': row['time_s'], 'stages': absent})
    # A bounded recorder can end with in-flight readbacks. Export the complete
    # prefix, retain the full original timeline and disclose the omitted tail.
    # Never fill missing stages with a different frame or pretend 15 s completed.
    if missing:
        duration = min(duration, missing[0]['time_s'])
        outputs = [row for row in outputs if row['time_s'] < duration]
    if not outputs or duration <= 0:
        raise ValueError('No complete matched output prefix is available; originals retained')
    fps = min(240, max(60, math.ceil(len(outputs) / max(duration, .001) / 30) * 30))
    clips, processes, progress = [], [], {'recording_id': rid, 'phase': 'exporting', 'completed_frames': 0}

    def save_progress(phase, done, total):
        progress.update(phase=phase, completed_frames=done, total_frames=total)
        (folder / 'export-progress.json').write_text(json.dumps(progress), encoding='utf-8')
        if shutil.disk_usage(folder).free < 1024**3:
            raise RuntimeError('Export stopped with less than 1 GiB disk free; compressed originals retained')

    def encoder(name, width, height, rate):
        path = folder / f'{rid}_{name}.mp4'
        log = (folder / f'encoder-{name}.log').open('wb')
        p = subprocess.Popen([str(ffmpeg), '-hide_banner', '-loglevel', 'error', '-f', 'rawvideo', '-pixel_format', 'bgra', '-video_size', f'{width}x{height}', '-framerate', str(rate), '-i', 'pipe:0', '-an', '-vf', 'pad=ceil(iw/2)*2:ceil(ih/2)*2', '-c:v', 'libx264', '-threads', '2', '-preset', 'veryfast', '-crf', '18', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', '-y', str(path)], stdin=subprocess.PIPE, stderr=log, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        processes.append((p, log))
        clips.append({'file': path.name, 'clip_id': path.stem, 'settings_id': session['settings_id'], 'kind': name, 'width': width, 'height': height, 'playback_fps': rate, 'lossless': False})
        return p

    def close_encoder(p):
        p.stdin.close()
        if p.wait() != 0:
            raise RuntimeError('Video export failed; see encoder log. Compressed originals retained.')

    try:
        # The original input and FG-only pixels remain available as indexed clips.
        for kind, name in [('R', 'raw-indexed'), ('G', 'generated-indexed')]:
            rows = [r for r in archive.rows if r['kind'] == kind]
            if not rows:
                continue
            p = encoder(name, w, h, len(rows) / max(result['duration_seconds'], .001))
            for n, row in enumerate(rows):
                if n % 30 == 0:
                    save_progress(name, n, len(rows))
                p.stdin.write(archive.frame((kind, row['id'])))
            close_encoder(p)
            clips[-1]['timing'] = 'Indexed source frames, evenly spaced. Exact original timing: frames.csv.'
        stages = {s: encoder(s, w, h, fps) for s in ('before', 'after', 'final')}
        tile_w = 640
        tile_h = round(h / w * tile_w)
        tile_h += tile_h % 2
        comparison = encoder('before-after-subtitles', tile_w * 3, tile_h + 40, fps)
        times = [r['time_s'] for r in outputs]
        black = bytes(w * h * 4)
        total = max(1, math.ceil(duration * fps))
        for n in range(total):
            if n % 30 == 0:
                save_progress('matched-output', n, total)
            i = bisect.bisect_right(times, n / fps) - 1
            row = outputs[i] if i >= 0 else None
            strip = Image.new('RGB', (tile_w * 3, tile_h + 40), '#10160f')
            draw = ImageDraw.Draw(strip)
            for col, (stage, p) in enumerate(stages.items()):
                pixels = archive.frame(row[stage]) if row else black
                p.stdin.write(pixels)
                image = Image.frombytes('RGBA', (w, h), pixels, 'raw', 'BGRA').convert('RGB')
                strip.paste(image.resize((tile_w, tile_h), Image.Resampling.BILINEAR), (col * tile_w, 40))
                draw.text((col * tile_w + 12, 12), {'before': 'BEFORE ENHANCER (includes FG)', 'after': 'AFTER ENHANCER', 'final': 'FINAL + CZECH SUBTITLES'}[stage], fill='white')
            comparison.stdin.write(strip.convert('RGBA').tobytes('raw', 'BGRA'))
        for p in [*stages.values(), comparison]:
            close_encoder(p)
        cues = subtitle_cues(outputs, duration)
        (folder / 'subtitles.cs.srt').write_text(''.join(f'{i+1}\n{srt_time(c["start_s"])} --> {srt_time(c["end_s"])}\n{c["text"]}\n\n' for i, c in enumerate(cues)), encoding='utf-8-sig')
        (folder / 'subtitles.json').write_text(json.dumps({'recording_id': rid, 'timing': 'Actual displayed Czech captions relative to recording start, includes translation delay; not inferred from OCR detection time.', 'cues': cues}, ensure_ascii=False, indent=2), encoding='utf-8')
        for clip in clips:
            clip['recording_id'] = rid
            with (folder / clip['file']).open('rb') as f:
                clip['sha256'] = hashlib.file_digest(f, 'sha256').hexdigest()
        manifest = {'schema': 2, 'recording_id': rid, 'settings_id': session['settings_id'], 'settings_file': 'session.json', 'pipeline_settings_file': 'pipeline-settings.json', 'settings_timeline': 'settings-timeline.jsonl', 'subtitle_settings_file': 'subtitle-settings.txt', 'nr_settings_file': 'nr-settings.txt', 'frames_file': 'frames.csv', 'output_timeline': 'output-frames.jsonl', 'subtitle_files': ['subtitles.cs.srt', 'subtitles.json'], 'subtitle_cues': len(cues), 'audio_recorded': False, 'exported_output_duration_s': duration, 'missing_output_pixels': missing, 'recorder_complete': not missing and result['recorder_drops'] == 0 and duration >= 14.99, 'archive': 'FLP1 lossless BGRA blocks in R/G/B/E/F.flp; random-access index in frames.csv; decode using tools/recording_archive.py + app/FrameLabPixels.dll', 'timing_note': 'Before/after/final use identical output IDs and app Present times, sampled to preview FPS with frame holds. No optical flow or new enhancement is applied during export. Before includes existing FG. Final is the clean source-sized fullscreen composition, without dashboard/cropping/comparison labels. This does not prove physical scanout FPS. Indexed raw/generated clips are not real-time output.', 'clips': clips, 'result': result}
        (folder / 'clips.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
        sections = ''.join(f'<h2>{html.escape(c["kind"])}</h2><video controls preload="none" src="{html.escape(c["file"])}"></video>' for c in clips)
        (folder / 'index.html').write_text('<!doctype html><meta charset="utf-8"><title>FrameLab recording</title><style>body{background:#10160f;color:#eee;font:18px system-ui;margin:30px}video{width:100%;max-height:75vh}a{color:#b6ff59}</style><h1>' + html.escape(rid) + '</h1><p><a href="clips.json">Settings + clip IDs</a> · <a href="subtitles.cs.srt">Czech subtitles (SRT)</a> · <a href="subtitles.json">Subtitle timing + source text</a></p><p>Same recorded frames, before enhancer / after enhancer / final with captions. Silent diagnostic clips. No desktop recording. Lossless source pixels remain in compressed FLP archives.</p>' + sections, encoding='utf-8')
        save_progress('complete', total, total)
        error_file = folder / 'export-error.txt'
        if error_file.exists():
            error_file.unlink()
    finally:
        for p, log in processes:
            if p.poll() is None:
                p.kill()
                p.wait()
            if p.stdin and not p.stdin.closed:
                try:
                    p.stdin.close()
                except OSError:
                    pass
            log.close()
        archive.close()
