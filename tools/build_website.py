"""Build the current site using explicit reviewed media; never deploy."""
from pathlib import Path
import argparse,hashlib,json,shutil,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'deps/python'))
from PIL import Image
def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'dist/framelab-web-preview15');p.add_argument('--downloads',type=Path);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    for source in (ROOT/'website').rglob('*'):
        if source.is_file() and source.name not in {'README.md','runtime-config.json'}:
            dest=out/source.relative_to(ROOT/'website');dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
    media=out/'media';media.mkdir(exist_ok=True);source=ROOT/'release-assets/preview15'
    names=['gta-2x-realtime.mp4','gta-2x-slow.mp4','gta-fg-pair.png','gta-fg-crop.png']
    for prefix,count in [('gta-enhancer',2),('gta-subtitles',3)]:
        for n in range(1,count+1):
            stem=f'{prefix}-{n}';names += [stem+'.mp4',stem+'-before.mp4',stem+'-after.mp4',stem+'-final.mp4',stem+'-before-full.png',stem+'-after-full.png',stem+'-final-full.png',stem+'-crop.png',stem+'.srt',stem+'-mapping.json']
    for name in names:shutil.copy2(source/name,media/name)
    shutil.copy2(ROOT/'docs/images/preview15/gta-2x-slow.gif',media/'gta-2x-slow.gif')
    shutil.copy2(ROOT/'docs/images/preview15/app.png',media/'app-preview15.png')
    for file in list(media.glob('*-full.png'))+[media/'gta-fg-pair.png',media/'app-preview15.png']:
        with Image.open(file) as im:
            im.thumbnail((1800,1200));im.convert('RGB').save(file.with_suffix('.webp'),quality=92)
    data=out/'data/debug';data.mkdir(parents=True,exist_ok=True)
    for name in ['summary.json','metrics.csv','captions.json']:shutil.copy2(source/'debug'/name,data/name)
    if a.downloads:
        public=[];downloads=out/'downloads';downloads.mkdir(exist_ok=True)
        for item in json.loads(a.downloads.read_text(encoding='utf-8-sig')):
            f=Path(item['path']);assert sha(f)==item['sha256'],f.name;shutil.copy2(f,downloads/f.name);public.append({'name':f.name,'bytes':f.stat().st_size,'sha256':item['sha256']})
        (downloads/'catalog.json').write_text(json.dumps({'version':'0.2.0-preview.15','files':public},indent=2),encoding='utf8')
        (downloads/'SHA256SUMS.txt').write_text(''.join(r['sha256']+'  '+r['name']+'\n' for r in public),encoding='ascii')
    manifest=[{'file':f.relative_to(out).as_posix(),'bytes':f.stat().st_size,'sha256':sha(f)} for f in sorted(out.rglob('*')) if f.is_file() and f.name!='site-manifest.json']
    (out/'site-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf8');print(json.dumps({'files':len(manifest),'bytes':sum(r['bytes'] for r in manifest),'output':str(out)}))
if __name__=='__main__':main()
