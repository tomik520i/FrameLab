"""Verify and read the public debug ZIP without extracting arbitrary paths."""
from pathlib import Path
import argparse,hashlib,json,zipfile

def main():
    p=argparse.ArgumentParser();p.add_argument('archive',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--manifest',type=Path,default=Path(__file__).resolve().parents[1]/'website/runs/2026-11-20-3h/data/source-manifest.json');a=p.parse_args()
    if a.output.exists():raise SystemExit('Use a new output file.')
    expected={x['file']:x for x in json.loads(a.manifest.read_text(encoding='utf-8')) if x['file'].startswith('events') and x['file'].endswith('.jsonl')}
    events=[];seen=set()
    with zipfile.ZipFile(a.archive) as z:
        for item in z.infolist():
            name=Path(item.filename).name
            if name not in expected:continue
            if name in seen or item.file_size!=expected[name]['bytes']:raise ValueError('Unexpected event member '+name)
            seen.add(name);sha=hashlib.sha256()
            for line in z.open(item):
                sha.update(line);e=json.loads(line)
                if e['event'] in ('ocr_complete','ocr_style_line','subtitle_reset'):events.append(e)
            if sha.hexdigest()!=expected[name]['sha256']:raise ValueError('Hash mismatch '+name)
    if seen!=set(expected):raise ValueError('Missing original event file')
    # ZIP's published order is preserved when equal timestamps occur, as in the
    # original experiment. All relevant event times are explicit in the stream.
    events.sort(key=lambda e:e['time_s']);a.output.parent.mkdir(parents=True,exist_ok=True)
    with a.output.open('w',encoding='utf-8') as f:
        for i,e in enumerate(events):e['stream_id']=i;f.write(json.dumps(e,ensure_ascii=False)+'\n')
    print(len(events),'verified chronological events')

if __name__=='__main__':main()
