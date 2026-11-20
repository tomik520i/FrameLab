"""Collect only reviewed current-recording evidence; preserve original takes."""
from pathlib import Path
import json, shutil, hashlib, sys, statistics
ROOT=Path(__file__).resolve().parents[1]

def main():
    originals,audits,media=map(Path,sys.argv[1:4]);out=ROOT/'examples/2026-11-20';out.mkdir(parents=True,exist_ok=True)
    entries=[]
    for take in sorted(audits.glob('analysis-*')):
        if not (take/'audit.json').exists():continue
        dest=out/take.name;dest.mkdir(exist_ok=True)
        for name in ['audit.json','generated-pairs.json','raw-changes.json']:
            shutil.copy2(take/name,dest/name)
        for name in ['session.json','result.json','pipeline-settings.json','settings-timeline.jsonl','capture-settings.json','subtitle-settings.txt','nr-settings.txt']:
            source=originals/take.name/name
            if source.exists():shutil.copy2(source,dest/name)
        d=json.loads((take/'audit.json').read_text())
        entries.append({'recording_id':take.name,'duration_s':d['result']['duration_seconds'],'matched_prefix_s':d['usable_prefix_end_s'],'raw_frames':d['result']['raw_frames'],'raw_changes':d['raw_changed_frames'],'generated_frames':d['result']['generated_frames'],'generated_pairs_checked':d['valid_generated_pairs'],'endpoint_copies':d['generated_endpoint_copies'],'submitted_image_ids':d['submitted_image_ids'],'subtitle_cues':len(d['subtitle_cues']),'recorder_drops':d['result']['recorder_drops'],'reason':d['result']['reason']})
    for file in media.glob('*-mapping.json'):shutil.copy2(file,out/file.name)
    for file in media.glob('*.srt'):shutil.copy2(file,out/file.name)
    for name in ['summary.json','captions.json','settings-timeline.json','metrics.csv']:
        dest=out/'debug';dest.mkdir(exist_ok=True);shutil.copy2(media/'debug'/name,dest/name)
    assets=[]
    for file in sorted(media.rglob('*')):
        if file.is_file():
            with file.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
            assets.append({'file':file.relative_to(media).as_posix(),'bytes':file.stat().st_size,'sha256':digest})
    (out/'manifest.json').write_text(json.dumps({'takes':entries,'media':assets,'pixel_source':'Original user recordings. No replacement interpolation, enhancement or translation during export. MP4/GIF are lossy previews; PNGs are exact selected pixels except labeled layouts and explicitly scaled overview panels.'},indent=2),encoding='utf8')
    print(json.dumps(entries,indent=2))

if __name__=='__main__':main()
