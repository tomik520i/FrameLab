"""Audit user takes without modifying them; prepare frame contact sheets for selection."""
from pathlib import Path
import sys, json, csv, hashlib, statistics, math, os
from collections import OrderedDict
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'deps/python'),str(ROOT/'tools')]
from PIL import Image,ImageDraw,ImageFont
import numpy as np
from recording_archive import Archive
from export_pipeline import subtitle_cues

class Take:
 def __init__(self,path):
  self.path=Path(path);self.settings=json.loads((self.path/'session.json').read_text());self.result=json.loads((self.path/'result.json').read_text());self.w=self.settings['width'];self.h=self.settings['height']
  decoder=Path(os.environ.get('FRAMELAB_PIXELS_DLL',str(ROOT/'app/FrameLabPixels.dll')))
  self.archive=Archive(self.path,decoder) if (self.path/'R.flp').exists() else None
  self.rows=self.archive.rows if self.archive else list(csv.DictReader((self.path/'frames.csv').open()))
  if not self.archive:
   for r in self.rows:
    for k in ('id','source_A','source_B','width','height','byte_offset','byte_count'):r[k]=int(r[k])
    r['time_s']=float(r['time_s'])
  self.lookup={(r['kind'],r['id']):r for r in self.rows};self.files={};self.cache=OrderedDict()
  self.outputs=[json.loads(s) for s in (self.path/'output-frames.jsonl').read_text(encoding='utf-8').splitlines()] if self.archive else []
 def pixels(self,key):
  if self.archive:return self.archive.frame(key)
  if key in self.cache:self.cache.move_to_end(key);return self.cache[key]
  k,i=key;r=self.lookup[k,i]
  if k not in self.files:self.files[k]=(self.path/('raw.bgra' if k=='R' else 'generated.bgra')).open('rb')
  f=self.files[k];f.seek(r['byte_offset']);data=f.read(r['byte_count'])
  if len(data)!=r['byte_count']:raise ValueError('Truncated source pixels')
  self.cache[key]=data
  while len(self.cache)>8:self.cache.popitem(last=False)
  return data
 def array(self,key):return np.frombuffer(self.pixels(key),dtype=np.uint8).reshape(self.h,self.w,4)
 def image(self,key):return Image.frombytes('RGBA',(self.w,self.h),self.pixels(key),'raw','BGRA').convert('RGB')
 def close(self):
  if self.archive:self.archive.close()
  for f in self.files.values():f.close()

def main():
 source=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True);reports=[]
 font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',18)
 for folder in sorted(source.glob('analysis-*')):
  t=Take(folder);dest=out/folder.name;dest.mkdir(exist_ok=True);raw=[r for r in t.rows if r['kind']=='R'];gen=[r for r in t.rows if r['kind']=='G'];hashes={};changed=[];last=None;pairmetrics=[]
  (out/'progress.json').write_text(json.dumps({'phase':'hashing','take':folder.name,'completed_takes':len(reports)}))
  for r in raw+gen:
   digest=hashlib.sha256(t.array((r['kind'],r['id']))[:,:,:3].tobytes()).hexdigest();hashes[r['kind'],r['id']]=digest
   if r['kind']=='R':
    if digest!=last:changed.append(r)
    last=digest
  for r in gen:
   a=t.lookup.get(('R',r['source_A']));b=t.lookup.get(('R',r['source_B']))
   if not a or not b:continue
   aa=t.array(('R',a['id']))[::8,::8,:3].astype(np.float32);bb=t.array(('R',b['id']))[::8,::8,:3].astype(np.float32);gg=t.array(('G',r['id']))[::8,::8,:3].astype(np.float32)
   pairmetrics.append({'id':r['id'],'a':a['id'],'b':b['id'],'time_s':r['time_s'],'source_mid_s':a['time_s']+(b['time_s']-a['time_s'])*float(r['timestep']),'phase':float(r['timestep']),'source_gap_ms':(b['time_s']-a['time_s'])*1000,'copy_a':hashes['G',r['id']]==hashes['R',a['id']],'copy_b':hashes['G',r['id']]==hashes['R',b['id']],'motion_mae':float(np.abs(aa-bb).mean()),'vs_a_mae':float(np.abs(gg-aa).mean()),'vs_b_mae':float(np.abs(gg-bb).mean())})
  valid=[];missing=[]
  for r in t.outputs:
   absent=[s for s in ('before','after','final') if tuple(r[s]) not in t.lookup]
   (missing if absent else valid).append({**r,'missing':absent} if absent else r)
  duration=t.result['duration_seconds'];bins=[]
  pres=list(csv.DictReader((folder/'presents.csv').open()));changes=[];prev=None
  for p in pres:
   k=(p['kind'].upper(),int(p['image_id']));p['present_time_s']=float(p['present_time_s']);p['image_id']=int(p['image_id'])
   if k!=prev:changes.append(p)
   prev=k
  for s in range(math.ceil(duration)):
   bins.append({'second':s,'raw':sum(s<=r['time_s']<s+1 for r in raw),'raw_changes':sum(s<=r['time_s']<s+1 for r in changed),'generated':sum(s<=r['time_s']<s+1 for r in gen),'submitted_ids':sum(s<=r['present_time_s']<s+1 for r in changes)})
  enhanced=[]
  for r in valid[::max(1,len(valid)//36)]:
   before=t.array(r['before'])[::4,::4,:3].astype(np.float32);after=t.array(r['after'])[::4,::4,:3].astype(np.float32);d=np.abs(after-before)
   enhanced.append({'time_s':r['time_s'],'output_id':r['output_id'],'mean_abs_rgb_change':float(d.mean()),'pixel_fraction_change_over_10':float((d.max(2)>10).mean()),'subtitle':r.get('subtitle','')})
  cues=subtitle_cues(valid,min(duration,missing[0]['time_s']) if missing else duration)
  report={'recording_id':folder.name,'settings':t.settings,'result':t.result,'raw_changed_frames':len(changed),'raw_adjacent_repeats':len(raw)-len(changed),'valid_generated_pairs':len(pairmetrics),'generated_endpoint_copies':sum(r['copy_a'] or r['copy_b'] for r in pairmetrics),'submitted_image_ids':len(changes),'valid_output_rows':len(valid),'missing_output_rows':missing,'usable_prefix_end_s':missing[0]['time_s'] if missing else duration,'subtitle_cues':cues,'bins':bins,'enhanced_sampled_changes':enhanced,'limitations':['Captured pixel changes are not engine FPS.','No true intermediate frame or enhanced ground truth: changes are not an accuracy score.','Recording load can reduce cadence. Presentation is not physical scanout.']}
  (dest/'audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');(dest/'generated-pairs.json').write_text(json.dumps(pairmetrics));(dest/'raw-changes.json').write_text(json.dumps(changed));(dest/'valid-outputs.json').write_text(json.dumps(valid,ensure_ascii=False),encoding='utf-8')
  count=min(15,math.ceil(duration));sheet=Image.new('RGB',(1536,math.ceil(count/3)*318),'#10160f');draw=ImageDraw.Draw(sheet)
  for s in range(count):
   row=min(raw,key=lambda r:abs(r['time_s']-s));x=(s%3)*512;y=(s//3)*318;sheet.paste(t.image(('R',row['id'])).resize((512,288)),(x,y+30));draw.text((x+8,y+5),f'{folder.name} / {s}s',font=font,fill='white')
  sheet.save(dest/'timeline.jpg',quality=88)
  if cues:
   sheet=Image.new('RGB',(1280,max(1,len(cues))*385),'#10160f');draw=ImageDraw.Draw(sheet)
   for i,c in enumerate(cues):
    row=min(valid,key=lambda r:abs(r['time_s']-(c['start_s']+.15)));sheet.paste(t.image(row['final']).resize((640,360)),(0,i*385+25));draw.text((648,i*385+30),f'{c["start_s"]:.2f}–{c["end_s"]:.2f}s\n'+c['source']+'\n\n'+c['text'],font=font,fill='white');draw.text((8,i*385+2),folder.name,font=font,fill='#b6ff59')
   sheet.save(dest/'subtitle-sheet.jpg',quality=92)
  reports.append({'recording_id':folder.name,'duration':duration,'usable_prefix':report['usable_prefix_end_s'],'valid_output_rows':len(valid),'missing':len(missing),'subtitle_cues':len(cues),'raw_changes':len(changed),'generated':len(gen),'copies':report['generated_endpoint_copies']})
  print(json.dumps(reports[-1]),flush=True);t.close()
 (out/'inventory.json').write_text(json.dumps(reports,indent=2));(out/'progress.json').write_text(json.dumps({'phase':'audited','takes':reports}));print('Audit complete',flush=True)

if __name__=='__main__':main()
