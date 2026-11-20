"""Verify actual offscreen compositor pixels and export subtitle timing."""
import sys, json, shutil, time
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
sys.path.insert(0, str(ROOT/'deps/python'))
from recording_archive import Archive
from export_pipeline import export, subtitle_cues
folder, decoder = Path(sys.argv[1]), Path(sys.argv[2])
a = Archive(folder, decoder)
rows = [json.loads(x) for x in (folder/'output-frames.jsonl').read_text(encoding='utf-8').splitlines()]
assert len(rows) == 90
for i, row in enumerate(rows):
    before, after, final = [a.frame(row[k]) for k in ('before', 'after', 'final')]
    assert before[1:4] == bytes([30,40,255])
    assert abs(after[1]-70) <= 1 and abs(after[2]-60) <= 1 and after[3] == 255, (i,after[:4])
    assert before != after
    assert final[:640*4*100] == after[:640*4*100], 'Caption changed unrelated top pixels'
    if 30 <= i < 60:
        assert final == after and not row['subtitle'], 'Expired caption remained burned in'
    else:
        assert final != after and row['subtitle'], 'Caption missing from recorded pixels'
result = json.loads((folder/'result.json').read_text())
cues = subtitle_cues(rows, result['duration_seconds'])
assert len(cues) == 2
assert cues[0]['end_s'] == rows[30]['time_s'] and cues[1]['start_s'] == rows[60]['time_s']
assert '\n' in cues[0]['text'] and 'český' in cues[0]['text']
assert cues[1]['end_s'] == result['duration_seconds']
a.close()
ffmpeg = next((ROOT/'deps/python/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
export(folder, ffmpeg, decoder)
manifest = json.loads((folder/'clips.json').read_text())
assert manifest['subtitle_cues'] == 2 and len(manifest['clips']) == 5
assert not list(folder.glob('*.bgra'))
(folder/'verification.json').write_text(json.dumps({'passed': True, 'checks': ['90 matched original/enhanced/final frames', '50% effect strength matches GPU pixels', 'untouched source area preserved', 'two-line Czech captions burned in', 'caption cleared and later caption displayed', 'SRT uses display times, including delay', 'compressed archives only, no BGRA scratch', 'five playable exports and settings manifest'], 'test_is_synthetic': True}, indent=2))
print('PASS: pixels, subtitle transitions, SRT timing, compressed archive and exports')
# Simulate a final GPU readback that never arrived. Keep the good prefix paired,
# expose the missing pixels, and do not speed up the indexed RAW track.
partial = folder.parent / ('partial-export-test-' + str(time.time_ns()))
partial.mkdir()
for source in folder.iterdir():
    if source.suffix in ('.flp', '.csv', '.json', '.jsonl', '.txt'):
        shutil.copy2(source, partial/source.name)
original_duration = result['duration_seconds'] + .2
result['duration_seconds'] = original_duration
(partial/'result.json').write_text(json.dumps(result))
missing = dict(rows[-1], output_id=999, time_s=original_duration-.1,
               before=['R', 999], after=['E', 999], final=['F', 999])
(partial/'output-frames.jsonl').write_text('\n'.join(json.dumps(r, ensure_ascii=False) for r in [*rows,missing]), encoding='utf-8')
export(partial, ffmpeg, decoder)
manifest = json.loads((partial/'clips.json').read_text())
assert manifest['exported_output_duration_s'] == missing['time_s']
assert not manifest['recorder_complete'] and len(manifest['missing_output_pixels']) == 1
assert abs(manifest['clips'][0]['playback_fps'] - 90/original_duration) < 1e-9
assert json.loads((partial/'subtitles.json').read_text(encoding='utf-8'))['cues'][-1]['end_s'] == missing['time_s']
print('PASS: incomplete GPU tail disclosed, complete prefix exported, RAW speed and SRT boundary preserved')
