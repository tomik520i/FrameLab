"""Prepare compact, explicit release assets without modifying the original take."""
import sys,json,shutil,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1];take=Path(sys.argv[1]).resolve()
manifest=json.loads((take/'clips.json').read_text());out=root/'release-assets'/take.name
out.mkdir(parents=True,exist_ok=True);assets=[]
for clip in manifest['clips']:
 if clip['kind'] not in ('output-preview','matched-A-generated-B'):continue
 source=take/clip['file']
 if source.parent!=take or not source.is_file():raise ValueError('Invalid clip path')
 target=out/source.name;shutil.copy2(source,target)
 assets.append(dict(clip,sha256=hashlib.sha256(target.read_bytes()).hexdigest(),bytes=target.stat().st_size))
for name in ['session.json','capture-settings.json','result.json','frames.csv','presents.csv']:
 shutil.copy2(take/name,out/name)
metadata={'recording_id':take.name,'settings_id':manifest['settings_id'],'clips':assets,'note':'Compressed previews, not lossless originals or monitor scanout. Backend missing in old metadata means legacy NVIDIA VFG. No changes applied to original take.'}
(out/'manifest.json').write_text(json.dumps(metadata,indent=2),encoding='utf-8')
print(out)
