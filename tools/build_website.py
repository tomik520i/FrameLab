"""Assemble the static Opicebot /framelab site from verified local evidence."""
from pathlib import Path
import argparse, hashlib, json, shutil, subprocess, sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'deps/python'))
from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'dist/framelab-web')
    parser.add_argument('--downloads', type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    for file in (ROOT/'website').rglob('*'):
        if file.is_file() and file.name != 'README.md':
            dest=out/file.relative_to(ROOT/'website');dest.parent.mkdir(parents=True, exist_ok=True);shutil.copy2(file,dest)
    media=out/'media';media.mkdir(exist_ok=True)
    ffmpeg=next((ROOT/'deps/python/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
    for key,rid in [('2x','82183953'),('3x','112947656'),('combat','113768562'),('4x','84744531')]:
        source=ROOT/f'release-assets/analysis-15s-{rid}/analysis-15s-{rid}_source-aligned_3s_4x-slow.mp4'
        shutil.copy2(source,media/f'{key}-slow.mp4')
        subprocess.run([str(ffmpeg),'-hide_banner','-v','error','-ss','6.6','-i',str(source),'-frames:v','1',
                        '-vf','scale=1600:-2','-quality','88','-y',str(media/f'{key}-poster.webp')],check=True)
    shutil.copy2(media/'2x-poster.webp',media/'hero-poster.webp')
    for name in ['framelab-2x-slow-comparison.gif','framelab-3x-slow-comparison.gif']:
        shutil.copy2(ROOT/'docs/images'/name,media/name)
    full=ROOT/'docs/images/framelab-live-ui.png'
    shutil.copy2(full,media/'app-full.png')
    with Image.open(full) as im:
        im.thumbnail((1800,1200));im.convert('RGB').save(media/'app-preview.webp',quality=88)
    phases=ROOT/'examples/113768562/phases-27164-27167_character.png'
    shutil.copy2(phases,media/'combat-phases.png')
    with Image.open(phases) as im:
        im.thumbnail((1600,800));im.convert('RGB').save(media/'combat-phases.webp',quality=94)
    # All download assets are copied explicitly from a reviewed catalog, never a directory glob.
    if args.downloads:
        files=json.loads(args.downloads.read_text(encoding='utf-8-sig'))
        target=out/'downloads';target.mkdir(exist_ok=True)
        public=[]
        for record in files:
            src=Path(record['path']);dest=target/src.name
            with src.open('rb') as f: actual=hashlib.file_digest(f,'sha256').hexdigest()
            assert actual==record['sha256'], src.name
            shutil.copy2(src,dest)
            public.append({'name':src.name,'bytes':src.stat().st_size,'sha256':actual})
        (target/'catalog.json').write_text(json.dumps({'version':'0.2.0-preview.8','files':public},indent=2),encoding='utf-8')
    manifest=[]
    for p in sorted(out.rglob('*')):
        if p.is_file() and p.name!='site-manifest.json':
            with p.open('rb') as f: digest=hashlib.file_digest(f,'sha256').hexdigest()
            manifest.append({'file':p.relative_to(out).as_posix(),'bytes':p.stat().st_size,'sha256':digest})
    (out/'site-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print(json.dumps({'output':str(out),'files':len(manifest),'bytes':sum(f['bytes'] for f in manifest)}))


if __name__=='__main__':main()
