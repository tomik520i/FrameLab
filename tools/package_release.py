"""Package a reviewed complete portable base and exact Git source; never upload."""
from pathlib import Path
import argparse
import hashlib
import html
import json
import shutil
import subprocess
import zipfile

ROOT=Path(__file__).resolve().parents[1]

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()

def publishable(path,base):
    rel=path.relative_to(base)
    return path.is_file() and not set(rel.parts).intersection({'logs','__pycache__','nr-cache','.git'}) and path.suffix!='.pyc' and path.name not in {'nr-settings.txt','subtitle-settings.txt','display-settings.txt','audio-settings.txt','imgui.ini','SHA256SUMS.txt'}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--base',type=Path,required=True)
    p.add_argument('--application',type=Path,required=True)
    p.add_argument('--build-cache',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--version',default='0.2.0-preview.10')
    p.add_argument('--commit',required=True)
    a=p.parse_args()
    commit=subprocess.check_output(['git','rev-parse',a.commit],cwd=ROOT,text=True).strip()
    subprocess.run(['git','diff','--exit-code',commit,'--'],cwd=ROOT,check=True,stdout=subprocess.DEVNULL)
    cache=a.build_cache.read_text(encoding='utf-8')
    assert 'FRAMELAB_EXPERIMENTAL_NR:BOOL=OFF' in cache,'Public build must have NR disabled'
    assert not list(a.base.rglob('nvngx_dlssnr.dll')) and not (a.base/'app/nr-host').exists()
    variant=json.loads((a.base/'build-variant.json').read_text(encoding='utf-8'))
    assert variant['FRAMELAB_EXPERIMENTAL_NR'] is False
    a.output.mkdir(parents=True,exist_ok=True)
    name='FrameLab-'+a.version
    stage=a.output/name
    assert not stage.exists(),'Choose a fresh output directory'
    for source in a.base.rglob('*'):
        if publishable(source,a.base):
            dest=stage/source.relative_to(a.base)
            dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source,dest)
    shutil.copy2(a.application,stage/'app/FrameLab.exe')
    for file in ['README.md','START_HERE.md','INSTALL.cs.md','THIRD_PARTY_NOTICES.md','LICENSE']:
        shutil.copy2(ROOT/file,stage/file)
    for source in (ROOT/'docs').rglob('*'):
        if publishable(source,ROOT/'docs'):
            dest=stage/'docs'/source.relative_to(ROOT/'docs')
            dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source,dest)
    for source in ROOT.glob('*.cmd'):
        if source.name.startswith(('Start FrameLab','1 - ','2 - ','3 - ','Ukazka - ')):
            shutil.copy2(source,stage/source.name)
    for file in ['prepare_translation.py','translate_worker.py','package_release.py']:
        shutil.copy2(ROOT/'tools'/file,stage/'tools'/file)
    shutil.copy2(ROOT/'docs/runtime-distribution.md',stage/'licenses/README.md')
    for sample in ['action','combat','ocr-demo']:
        assert (stage/'app/samples'/f'{sample}.mp4').stat().st_size>100000
        meta=stage/'app/samples'/f'{sample}.source.json'
        info=json.loads(meta.read_text(encoding='utf-8'))
        if 'sha256' in info:
            assert info['sha256']==digest(stage/'app/samples'/f'{sample}.mp4')
        if sample=='action':
            info['rights']='Recording public domain per Xiph/Twitch notice; underlying game content remains copyrighted and subject to applicable fair use. Not relicensed as GPL.'
            info['rights_source']='https://media.xiph.org/video/derf/twitch/copyright.txt'
        elif sample=='combat':
            info['rights']='Short excerpt of project owner previously published gameplay capture; game content retains respective rights. Not relicensed as GPL. See docs/sample-provenance.md.'
        meta.write_text(json.dumps(info,indent=2,ensure_ascii=False),encoding='utf-8')
    shutil.copy2(ROOT/'docs/sample-xiph-copyright.txt',stage/'app/samples/XIPH-COPYRIGHT.txt')
    for manifest in [stage/'app/runtime/manifest.json',stage/'licenses/microsoft-vc-manifest.json']:
        for item in json.loads(manifest.read_text(encoding='utf-8-sig'))['files']:
            assert digest(stage/'app/runtime'/item['name'])==item['sha256']
    translation=json.loads((stage/'app/translation/provenance.json').read_text(encoding='utf-8'))
    for file,sha in translation['model']['files'].items():
        assert digest(stage/'app/translation/model'/file)==sha,file
    assert (stage/'app/translation/python/python.exe').is_file()
    assert (stage/'tools/python/python.exe').is_file()
    guide=(ROOT/'INSTALL.cs.md').read_text(encoding='utf-8')
    (stage/'ZACNI TADY.txt').write_text(guide,encoding='utf-8-sig')
    (stage/'ZACNI TADY.html').write_text('<!doctype html><html lang="cs"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FrameLab – instalace</title><style>body{max-width:900px;margin:40px auto;padding:0 24px;background:#101511;color:#f1f3e9;font:17px/1.65 system-ui}h1{color:#c1ff5e}pre{white-space:pre-wrap;font:inherit}</style><h1>FrameLab: rozbalit a spustit</h1><p>1. Rozbal celý ZIP. 2. Otevři „1 - Spustit FrameLab.cmd“. 3. Vyzkoušej ukázky v kartě Vstup.</p><pre>'+html.escape(guide)+'</pre></html>',encoding='utf-8')
    (stage/'README-v3.txt').write_text('This is the public '+a.version+' full package. Start with ZACNI TADY.html, INSTALL.cs.md or START_HERE.md. NR is excluded.\n',encoding='utf-8')
    metadata={'version':a.version,'source_commit':commit,'FRAMELAB_EXPERIMENTAL_NR':False,'application_sha256':digest(stage/'app/FrameLab.exe'),'bundled_translation':True,'bundled_game_samples':2,'nr_runtime_included':False}
    (stage/'build-variant.json').write_text(json.dumps(metadata,indent=2),encoding='utf-8')
    files=sorted(x for x in stage.rglob('*') if publishable(x,stage))
    (stage/'SHA256SUMS.txt').write_text(''.join(digest(x)+'  '+x.relative_to(stage).as_posix()+'\n' for x in files),encoding='utf-8')
    portable=a.output/(name+'-windows-x64-full.zip')
    with zipfile.ZipFile(portable,'w',zipfile.ZIP_DEFLATED,compresslevel=5) as z:
        for file in files+[stage/'SHA256SUMS.txt']:
            z.write(file,name+'/'+file.relative_to(stage).as_posix())
    source=a.output/(name+'-source.zip')
    subprocess.run(['git','archive','--format=zip','--output='+str(source.resolve()),commit],cwd=ROOT,check=True)
    # Upstream dependency source, exact version used by this build.
    with zipfile.ZipFile(source,'a',zipfile.ZIP_DEFLATED,compresslevel=5) as z:
        existing=set(z.namelist())
        for file in (ROOT/'third_party/imgui').rglob('*'):
            if publishable(file,ROOT/'third_party/imgui') and file.relative_to(ROOT).as_posix() not in existing:
                z.write(file,file.relative_to(ROOT).as_posix())
    records=[]
    for archive in [portable,source]:
        with zipfile.ZipFile(archive) as z:
            assert z.testzip() is None
            assert not any(x.lower().endswith('nvngx_dlssnr.dll') or '/nr-host/' in x.lower() for x in z.namelist())
        records.append({'name':archive.name,'bytes':archive.stat().st_size,'sha256':digest(archive)})
    (a.output/'SHA256SUMS.txt').write_text(''.join(x['sha256']+'  '+x['name']+'\n' for x in records),encoding='ascii')
    (a.output/'release-provenance.json').write_text(json.dumps({**metadata,'archives':records},indent=2),encoding='utf-8')
    print(json.dumps(records,indent=2),flush=True)

if __name__=='__main__':
    main()
