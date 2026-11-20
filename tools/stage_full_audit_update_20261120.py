"""Stage only report/homepage changes with prior-file hashes; no deployment."""
import hashlib,json,shutil,subprocess,tarfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
WORK=ROOT/'analysis-output/prompt-retune-debug-20261120/publication-final'
STAGE=WORK/'site';STAGE.mkdir(parents=True,exist_ok=True)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
previous=json.loads(subprocess.check_output(['ssh','-o','BatchMode=yes','tomik520i@192.168.1.175','cat /var/www/html/framelab/site-manifest.json'],text=True))
prev={x['file']:x for x in previous};items=[]
files=[ROOT/'website/index.html']+[x for x in (ROOT/'website/runs/2026-11-20-3h').rglob('*') if x.is_file()]
sources={x.relative_to(ROOT/'website').as_posix():x for x in files}
sources['runs/2026-11-20-3h/data/analysis-full1747.zip']=ROOT/'release-assets/debug-run-1795196244267574/analysis-full1747.zip'
sources['runs/2026-11-20-3h/data/prompt-test-evidence.zip']=ROOT/'release-assets/debug-run-1795196244267574/prompt-test-evidence.zip'
for rel,source in sources.items():
 digest=sha(source)
 if prev.get(rel,{}).get('sha256')==digest:continue
 dest=STAGE/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
 items.append(dict(file=rel,sha256=digest,bytes=source.stat().st_size,previous_sha256=prev.get(rel,{}).get('sha256')))
(WORK/'manifest.json').write_text(json.dumps(items,indent=2),encoding='utf-8')
with tarfile.open(WORK/'update.tar','w') as tf:
 for r in items:tf.add(STAGE/r['file'],arcname='site/'+r['file'])
 tf.add(WORK/'manifest.json',arcname='manifest.json')
print(json.dumps(dict(files=len(items),bytes=sum(x['bytes'] for x in items),archive=str(WORK/'update.tar'))))
