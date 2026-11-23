"""Package a reviewed complete portable base and exact Git source; never upload."""
from pathlib import Path
import argparse
import hashlib
import html
import json
import shutil
import subprocess
import zipfile
from package_guides import copy_guides

ROOT=Path(__file__).resolve().parents[1]

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()

def publishable(path,base):
    rel=path.relative_to(base)
    return path.is_file() and not set(rel.parts).intersection({'logs','recordings','__pycache__','nr-cache','.git','subtitle-style-samples'}) and path.suffix!='.pyc' and path.name not in {'recording-folder.txt','recording-folder.txt.tmp','runtime-session.txt','runtime-session.txt.tmp','nr-settings.txt','subtitle-settings.txt','subtitle-profiles.txt','subtitle-profiles.txt.tmp','display-settings.txt','gpu-settings.txt','gpu-settings.txt.tmp','openai-settings.txt','openai-settings.txt.tmp','window-overlay-settings.txt','audio-settings.txt','file-audio-settings.txt','imgui.ini','SHA256SUMS.txt'}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--base',type=Path,required=True)
    p.add_argument('--application',type=Path,required=True)
    p.add_argument('--build-cache',type=Path,required=True)
    p.add_argument('--nr-host',type=Path,required=True)
    p.add_argument('--setup',type=Path,required=True)
    p.add_argument('--ocr',type=Path,required=True,help='Verified output of prepare_ocr.py')
    p.add_argument('--media',type=Path,required=True,help='Verified output of prepare_media.py')
    p.add_argument('--rife',type=Path,required=True,help='Verified runtime/rife folder, including all three ONNX models and licenses')
    p.add_argument('--entity-filter',type=Path,help='Prepared translation/filter directory, including tokenizers dependencies and licenses')
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--version',default='0.2.0-preview.18')
    p.add_argument('--commit',required=True)
    p.add_argument('--gta-sample',type=Path,required=True)
    a=p.parse_args()
    commit=subprocess.check_output(['git','rev-parse',a.commit],cwd=ROOT,text=True).strip()
    subprocess.run(['git','diff','--exit-code',commit,'--'],cwd=ROOT,check=True,stdout=subprocess.DEVNULL)
    cache=a.build_cache.read_text(encoding='utf-8')
    assert 'FRAMELAB_EXPERIMENTAL_NR:BOOL=ON' in cache,'Build must include the SDK-free NR adapter'
    assert not list(a.base.rglob('nvngx_dlssnr.dll')),'External NR model runtime is not a public packaging dependency'
    assert not list(a.base.rglob('*.gguf')) and not (a.base/'app/translation/gpu-runtime').exists(),'Use a clean public base without private GPU downloads'
    variant=json.loads((a.base/'build-variant.json').read_text(encoding='utf-8'))
    assert variant['FRAMELAB_EXPERIMENTAL_NR'] is False or (variant.get('nr_runtime_included') is False and variant.get('nr_host_sdk_static_library') is False),'Base must have no external NR runtime or SDK static library'
    a.output.mkdir(parents=True,exist_ok=True)
    name='FrameLab-'+a.version
    stage=a.output/name
    assert not stage.exists(),'Choose a fresh output directory'
    for source in a.base.rglob('*'):
        if publishable(source,a.base) and source.name not in {'combat.mp4','combat.source.json','Ukazka - Spider-Man.cmd'}:
            dest=stage/source.relative_to(a.base)
            dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source,dest)
    shutil.copy2(a.application,stage/'app/FrameLab.exe')
    shutil.copy2(a.application.parent/'FrameLabPixels.dll',stage/'app/FrameLabPixels.dll')
    shutil.copy2(a.setup,stage/'app/FrameLab-Setup.exe')
    for file in ['export_analysis.py','export_pipeline.py','recording_archive.py']:
        shutil.copy2(ROOT/'tools'/file,stage/'tools'/file)
    ocr_manifest=json.loads((a.ocr/'provenance.json').read_text(encoding='utf-8'))
    for model in ocr_manifest['models']:
        assert digest(a.ocr/'models'/model['name'])==model['sha256'],model['name']
    for file in a.ocr.rglob('*'):
        if publishable(file,a.ocr):
            dest=stage/'app/ocr'/file.relative_to(a.ocr)
            dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(file,dest)
    shutil.copy2(ROOT/'tools/ocr_worker.py',stage/'app/ocr/ocr_worker.py')
    shutil.copy2(ROOT/'tools/ocr_style.py',stage/'app/ocr/ocr_style.py')
    media_pin=json.loads((ROOT/'docs/media-runtime.json').read_text(encoding='utf-8'))
    assert json.loads((a.media/'provenance.json').read_text(encoding='utf-8'))==media_pin,'Media runtime provenance mismatch'
    assert (a.media/'libvlc.dll').is_file() and (a.media/'libvlccore.dll').is_file() and (a.media/'plugins/demux/libmkv_plugin.dll').is_file(),'Incomplete media runtime'
    shutil.copytree(a.media,stage/'app/media/vlc',dirs_exist_ok=True)
    rife_manifest=json.loads((a.rife/'model-provenance.json').read_text(encoding='utf-8'))
    for model in rife_manifest['outputs']:
        assert digest(a.rife/model['file'])==model['sha256'],model['file']
    for required in ['onnxruntime.dll','onnxruntime_providers_shared.dll','DirectML.dll','licenses/DirectML-LICENSE.txt','licenses/RIFE-LICENSE','licenses/SmoovPlay-LICENSE']:
        assert (a.rife/required).is_file(),required
    shutil.copytree(a.rife,stage/'app/runtime/rife',dirs_exist_ok=True)
    if a.entity_filter:
        pin=json.loads((a.entity_filter/'provenance.json').read_text('utf-8'))
        for asset in pin['files']:assert digest(a.entity_filter/asset['file'])==asset['sha256'],asset['file']
        assert (a.entity_filter/'deps/tokenizers').is_dir() and (a.entity_filter/'licenses/Apache-2.0.txt').is_file(),'Incomplete entity filter'
        shutil.copytree(a.entity_filter,stage/'app/translation/filter',ignore=shutil.ignore_patterns('__pycache__','*.pyc'),dirs_exist_ok=True)
    (stage/'app/language.txt').write_text('0\n',encoding='ascii')
    (stage/'app/nr-host').mkdir(exist_ok=True)
    shutil.copy2(a.nr_host,stage/'app/nr-host/nvngx.dll')
    for file in ['README.md','START_HERE.md','INSTALL.cs.md','THIRD_PARTY_NOTICES.md','LICENSE']:
        shutil.copy2(ROOT/file,stage/file)
    for source in (ROOT/'docs').rglob('*'):
        if publishable(source,ROOT/'docs'):
            dest=stage/'docs'/source.relative_to(ROOT/'docs')
            dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source,dest)
    for source in ROOT.glob('*.cmd'):
        if source.name.startswith(('Start FrameLab','1 - ','2 - ','3 - ','4 - ','Ukazka - ')) and source.name!='Ukazka - Spider-Man.cmd':
            shutil.copy2(source,stage/source.name)
    for file in ['prepare_translation.py','prepare_ocr.py','prepare_media.py','prepare_entity_filter.py','ocr_worker.py','ocr_style.py','translate_worker.py','subtitle_models.py','subtitle_model_manager.py','subtitle_compute.py','subtitle_gpu.py','openai_translation.py','subtitle_filter.py','subtitle_entities.py','subtitle_languages.py','package_release.py','package_guides.py','Install-Enhancer.ps1']:
        shutil.copy2(ROOT/'tools'/file,stage/'tools'/file)
    for file in ['translate_worker.py','subtitle_models.py','subtitle_model_manager.py','subtitle_compute.py','subtitle_gpu.py','openai_translation.py','subtitle_filter.py','subtitle_entities.py','subtitle_languages.py']:
        shutil.copy2(ROOT/'tools'/file,stage/'app/translation'/file)
    shutil.copy2(ROOT/'docs/subtitle-models.json',stage/'app/translation/subtitle-models.json')
    shutil.copy2(ROOT/'docs/openai-model-profiles.json',stage/'app/translation/openai-model-profiles.json')
    for folder in ['translation','ocr']:
        shutil.copy2(ROOT/'docs/subtitle-languages.json',stage/'app'/folder/'subtitle-languages.json')
        shutil.copy2(ROOT/'tools/subtitle_languages.py',stage/'app'/folder/'subtitle_languages.py')
    for notice in ['Gemma-LICENSE.txt','Gemma-Notice.txt']:
        shutil.copy2(ROOT/'docs/licenses'/notice,stage/'app/translation'/notice)
    shutil.copy2(ROOT/'docs/runtime-distribution.md',stage/'licenses/README.md')
    shutil.copy2(a.gta_sample,stage/'app/samples/gta.mp4')
    shutil.copy2(ROOT/'docs/sample-gta-source.json',stage/'app/samples/gta.source.json')
    for sample in ['action','gta','ocr-demo']:
        assert (stage/'app/samples'/f'{sample}.mp4').stat().st_size>100000
        meta=stage/'app/samples'/f'{sample}.source.json'
        info=json.loads(meta.read_text(encoding='utf-8'))
        if 'sha256' in info:
            assert info['sha256']==digest(stage/'app/samples'/f'{sample}.mp4')
        if sample=='action':
            info['rights']='Recording public domain per Xiph/Twitch notice; underlying game content remains copyrighted and subject to applicable fair use. Not relicensed as GPL.'
            info['rights_source']='https://media.xiph.org/video/derf/twitch/copyright.txt'
        elif sample=='gta':
            info['rights']='Short excerpt of project owner gameplay capture; game content retains respective rights. Not relicensed as GPL. See docs/sample-provenance.md.'
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
    copy_guides(stage)
    (stage/'README-v3.txt').write_text('This is the public '+a.version+' full package. Start with ZACNI TADY.html, INSTALL.cs.md or START_HERE.md. NR adapter and import wizard included; external NR model DLLs are not bundled.\n',encoding='utf-8')
    metadata={'version':a.version,'source_commit':commit,'FRAMELAB_EXPERIMENTAL_NR':True,'setup_sha256':digest(stage/'app/FrameLab-Setup.exe'),'nr_host_sha256':digest(stage/'app/nr-host/nvngx.dll'),'nr_host_sdk_static_library':False,'application_sha256':digest(stage/'app/FrameLab.exe'),'bundled_translation':True,'bundled_ocr':True,'bundled_game_samples':2,'nr_runtime_included':False,'bundled_rife':'4.22 lite / DirectML','ocr_recognizers':12,'language_choices':40,'gpu_translation_models_included':False,'window_capture':True,'subtitle_only_overlay':True,'optional_openai_byok':True,'translation_backends':['CUDA','Vulkan','CPU'],'bundled_entity_filter':bool(a.entity_filter)}
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
            assert not any(x.lower().endswith(('nvngx_dlssnr.dll','.gguf')) for x in z.namelist())
        records.append({'name':archive.name,'bytes':archive.stat().st_size,'sha256':digest(archive)})
    (a.output/'SHA256SUMS.txt').write_text(''.join(x['sha256']+'  '+x['name']+'\n' for x in records),encoding='ascii')
    (a.output/'release-provenance.json').write_text(json.dumps({**metadata,'archives':records},indent=2),encoding='utf-8')
    print(json.dumps(records,indent=2),flush=True)

if __name__=='__main__':
    main()
