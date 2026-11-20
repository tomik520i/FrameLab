"""Prepare a scoped, hash-checked publication. Does not contact the web server."""
import hashlib,json,shutil,tarfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
WORK=ROOT/'analysis-output/debug-run-1795196244267574/publication'
STAGE=WORK/'site';STAGE.mkdir(parents=True,exist_ok=True)
def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
for relative in [Path('index.html'),*Path('website/runs/2026-11-20-3h').rglob('*')]:
    if relative==Path('index.html'):source=ROOT/'website'/relative;dest=STAGE/relative
    else:
        source=ROOT/relative
        if not source.is_file():continue
        dest=STAGE/relative.relative_to('website')
    dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
for name in ['raw-debug-1795196244267574.zip','analysis.zip']:
    shutil.copy2(ROOT/'release-assets/debug-run-1795196244267574'/name,STAGE/'runs/2026-11-20-3h/data'/name)
manifest=[dict(file=p.relative_to(STAGE).as_posix(),bytes=p.stat().st_size,sha256=sha(p)) for p in sorted(STAGE.rglob('*')) if p.is_file()]
(WORK/'publish-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
for line in (STAGE/'runs/2026-11-20-3h/data/SHA256SUMS').read_text().splitlines():
    digest,name=line.split('  ',1);assert sha(STAGE/'runs/2026-11-20-3h/data'/name)==digest,name
with tarfile.open(WORK/'publication.tar','w') as tf:
    for m in manifest:tf.add(STAGE/m['file'],arcname='site/'+m['file'])
    tf.add(WORK/'publish-manifest.json',arcname='publish-manifest.json')
print(json.dumps(dict(files=len(manifest),bytes=sum(m['bytes'] for m in manifest),archive=str(WORK/'publication.tar'))))
