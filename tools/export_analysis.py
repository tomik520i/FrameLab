"""Create playable analysis clips; authoritative frame timing stays in CSV."""
import sys,csv,json,subprocess,statistics,hashlib,bisect,traceback
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'deps/python'))
from PIL import Image,ImageDraw
folder=Path(sys.argv[1]).resolve()
ffmpeg=next((ROOT/'deps/python/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
def run(args):
    subprocess.run([str(ffmpeg),'-hide_banner','-loglevel','error',*args],check=True)
def export():
    settings=json.loads((folder/'session.json').read_text());
    if (folder/'capture-settings.json').exists():settings.update(json.loads((folder/'capture-settings.json').read_text()))
    result=json.loads((folder/'result.json').read_text())
    rows=list(csv.DictReader((folder/'frames.csv').open()));presents=list(csv.DictReader((folder/'presents.csv').open()))
    for r in rows:
        for k in ['id','source_A','source_B','width','height','byte_offset','byte_count']:r[k]=int(r[k])
        r['time_s']=float(r['time_s'])
    for r in presents:r['present_time_s']=float(r['present_time_s']);r['image_id']=int(r['image_id'])
    timeline=[0.]+[r['present_time_s'] for r in presents]+[result['duration_seconds']]
    largest_present_gap=max((b-a for a,b in zip(timeline,timeline[1:])),default=result['duration_seconds'])
    streams={k:[r for r in rows if r['kind']==k] for k in ['R','G']}
    w,h=settings['width'],settings['height'];clips=[];rid=settings['recording_id'];duration=max(.001,result['duration_seconds'])
    if any((r['width'],r['height'])!=(w,h) for r in rows):raise RuntimeError('Mixed dimensions; keep raw data for analysis')
    for kind,name in [('R','raw'),('G','generated')]:
        seq=streams[kind]
        if not seq:continue
        intervals=[b['time_s']-a['time_s'] for a,b in zip(seq,seq[1:]) if b['time_s']>a['time_s']]
        fps=len(seq)/duration
        file=folder/f'{rid}_{name}.mkv'
        run(['-f','rawvideo','-pixel_format','bgra','-video_size',f'{w}x{h}','-framerate',str(fps),'-i',str(folder/f'{name}.bgra'),'-an','-c:v','ffv1','-threads','2','-level','3','-pix_fmt','bgr0','-y',str(file)])
        clips.append({'clip_id':rid+'_'+name,'file':file.name,'settings_id':settings['settings_id'],'kind':kind,'frames':len(seq),'width':w,'height':h,'lossless':True,'playback_fps':fps,'timing':'Indexed frames evenly spaced over recorded duration; authoritative individual timestamps in frames.csv. No source frames removed.'})
    lookup={(r['kind'],r['id']):r for r in rows};files={'R':(folder/'raw.bgra').open('rb'),'G':(folder/'generated.bgra').open('rb')}
    cache={};missing=set()
    def frame(kind,id):
        key=(kind,id)
        if key in cache:return cache[key]
        row=lookup.get(key)
        if not row:
            missing.add(key);im=Image.new('RGB',(w,h));ImageDraw.Draw(im).text((20,20),'Frame outside recording / unavailable',fill='white');return im
        f=files[kind];f.seek(row['byte_offset']);data=f.read(row['byte_count'])
        if len(data)!=row['byte_count']:raise RuntimeError('Truncated recorded pixel data')
        im=Image.frombytes('RGBA',(w,h),data,'raw','BGRA').convert('RGB')
        if len(cache)>12:cache.clear()
        cache[key]=im;return im
    fps=60;output=folder/f'{rid}_output-preview.mp4';comparison=folder/f'{rid}_comparison.mp4'
    def encoder(path,size):return subprocess.Popen([str(ffmpeg),'-hide_banner','-loglevel','error','-f','rawvideo','-pixel_format','rgb24','-video_size',size,'-framerate',str(fps),'-i','pipe:0','-an','-c:v','libx264','-threads','2','-preset','fast','-crf','16','-pix_fmt','yuv420p','-movflags','+faststart','-y',str(path)],stdin=subprocess.PIPE)
    out=encoder(output,'1280x720');comp=encoder(comparison,'1920x400');pt=[r['present_time_s'] for r in presents];gt=[r['time_s'] for r in streams['G']]
    for n in range(max(1,round(duration*fps))):
        t=n/fps;p=bisect.bisect_right(pt,t)-1;gi=bisect.bisect_right(gt,t)-1
        image=frame(presents[p]['kind'].upper(),presents[p]['image_id']) if p>=0 else Image.new('RGB',(w,h))
        out.stdin.write(image.resize((1280,720),Image.Resampling.BILINEAR).tobytes())
        strip=Image.new('RGB',(1920,400),'#0c1420');d=ImageDraw.Draw(strip)
        if gi>=0:
            r=streams['G'][gi]
            for i,(kind,id,label) in enumerate([('R',r['source_A'],'RAW A'),('G',r['id'],'GENERATED'),('R',r['source_B'],'RAW B')]):
                strip.paste(frame(kind,id).resize((640,360),Image.Resampling.BILINEAR),(i*640,40));d.text((i*640+10,10),f'{label} | ID {id} | t={r["timestep"]}',fill='white')
        comp.stdin.write(strip.tobytes())
    for process in [out,comp]:
        process.stdin.close()
        if process.wait()!=0:raise RuntimeError('Preview encoding failed')
    for f in files.values():f.close()
    for file,kind,size in [(output,'output-preview',[1280,720]),(comparison,'matched-A-generated-B',[1920,400])]:
        clips.append({'clip_id':file.stem,'file':file.name,'settings_id':settings['settings_id'],'kind':kind,'width':size[0],'height':size[1],'lossless':False,'playback_fps':60,'timing':'Time-aligned preview sampled at 60 FPS; not every Present is included. Complete submission timeline is presents.csv; full generated pixels are in generated.bgra.'})
    for c in clips:
        c['recording_id']=rid
        with (folder/c['file']).open('rb') as f:c['sha256']=hashlib.file_digest(f,'sha256').hexdigest()
    manifest={'recording_id':rid,'settings_id':settings['settings_id'],'settings_file':'session.json','result_file':'result.json','frames_file':'frames.csv','present_timeline':'presents.csv','audio_recorded':False,'recorder_complete':result['recorder_drops']==0 and result['duration_seconds']>=14.99 and result['raw_frames']>0 and largest_present_gap<.25,'largest_present_gap_seconds':largest_present_gap,'timing_warning':'Application presentation stalled during recording' if largest_present_gap>=.25 else None,'capture_settings_file':'capture-settings.json' if (folder/'capture-settings.json').exists() else None,'clips':clips,'unavailable_referenced_images':[list(x) for x in sorted(missing)],'settings':settings,'result':result}
    if (folder/'display-confirmations.csv').exists():manifest['display_confirmations_file']='display-confirmations.csv';manifest['display_confirmation_note']='Sampled OS-confirmed image IDs; conservative lower bound, not physical scanout or pixel uniqueness.'
    (folder/'clips.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    (folder/'index.html').write_text('<!doctype html><meta charset="utf-8"><title>'+rid+'</title><style>body{background:#101820;color:white;font:18px system-ui;margin:30px}video{width:95%;max-height:70vh}a{color:#62ddbd}</style><h1>'+rid+'</h1><p><a href="clips.json">Clip IDs + settings JSON</a> | <a href="session.json">Session settings</a> | <a href="frames.csv">Every recorded frame</a></p><h2>Matched RAW A / generated / RAW B</h2><video controls src="'+comparison.name+'"></video><h2>Output timeline preview (60 FPS)</h2><video controls src="'+output.name+'"></video><p>Full-resolution lossless RAW and generated clips: '+ ' · '.join('<a href="'+c['file']+'">'+c['clip_id']+'</a>' for c in clips if c['lossless'])+'</p>',encoding='utf-8')
try:export()
except Exception:
    (folder/'export-error.txt').write_text(traceback.format_exc(),encoding='utf-8');raise
