"""Render reproducible authored scene pixels; never contact a translator."""
from pathlib import Path
import argparse,json,subprocess,hashlib,time

def stamp(t):
 n=round(t*100);return f'{n//360000}:{n//6000%60:02}:{n//100%60:02}.{n%100:02}'

def main():
 p=argparse.ArgumentParser();p.add_argument('experiment',type=Path);p.add_argument('--ffmpeg',type=Path,required=True);p.add_argument('--background',type=Path,required=True);a=p.parse_args();out=a.experiment.resolve();video=out/'scenes';video.mkdir(exist_ok=True)
 dataset=json.loads((out/'dataset.json').read_text(encoding='utf-8'));mapping={};scenes={}
 for c in dataset['cases']:
  payload=dict(source_language=c['source_language'],rows=c['rows'],duration_s=c['duration_s']);key=hashlib.sha256(json.dumps(payload,ensure_ascii=False,sort_keys=True).encode()).hexdigest()[:20];mapping[c['id']]=key;scenes[key]=payload
 (out/'scene-map.json').write_text(json.dumps(mapping,indent=2),encoding='utf-8')
 for i,(key,scene) in enumerate(scenes.items()):
  target=video/(key+'.mp4');font='Yu Gothic' if scene['source_language']=='ja' else 'Arial'
  ass=f'''[Script Info]
ScriptType: v4.00+
PlayResX: 1280
PlayResY: 720
WrapStyle: 0
[V4+ Styles]
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding
Style: Default,{font},30,&H00FFFFFF,&H00FFFFFF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,2,0,2,120,120,100,1
[Events]
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
'''
  for row in scene['rows']:ass+=f"Dialogue: 0,{stamp(row['start'])},{stamp(row['end'])},Default,,0,0,0,,{row['text']}\n"
  (video/(key+'.ass')).write_text(ass,encoding='utf-8')
  meta_path=video/(key+'.json');previous=json.loads(meta_path.read_text(encoding='utf-8')) if meta_path.exists() else {}
  valid=target.exists() and previous.get('encoder')=='h264_nvenc-p4-cq20' and hashlib.sha256(target.read_bytes()).hexdigest()==previous.get('sha256')
  if not valid:
   cmd=[str(a.ffmpeg.resolve()),'-hide_banner','-loglevel','error','-y','-stream_loop','-1','-i',str(a.background.resolve()),'-t',str(scene['duration_s']),'-vf',f'scale=1280:720,drawbox=x=0:y=520:w=1280:h=200:color=black:t=fill,ass={key}.ass','-an','-r','30','-c:v','h264_nvenc','-preset','p4','-cq','20','-pix_fmt','yuv420p',str(target)]
   subprocess.run(cmd,cwd=video,check=True)
  meta_path.write_text(json.dumps(dict(**scene,font=font,encoder='h264_nvenc-p4-cq20',sha256=hashlib.sha256(target.read_bytes()).hexdigest(),background_sha256=hashlib.sha256(a.background.read_bytes()).hexdigest()),ensure_ascii=False,indent=2),encoding='utf-8')
  (out/'render-progress.json').write_text(json.dumps(dict(done=i+1,total=len(scenes),phase='Rendering source videos',updated=time.time())),encoding='utf-8')
  print(f'Rendered {i+1}/{len(scenes)} ({scene["source_language"]})',flush=True)
 print('Ready:',len(scenes),'distinct source videos for',len(mapping),'translation cases.')
if __name__=='__main__':main()
