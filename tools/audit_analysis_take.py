"""Audit every recorded frame and submission; pixel distance is not ground-truth quality."""
from pathlib import Path
import sys,csv,json,hashlib,statistics,math
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'deps/python'))
import numpy as np
from PIL import Image,ImageDraw,ImageFont
take=Path(sys.argv[1]).resolve();out=ROOT/'analysis-output'/take.name;out.mkdir(parents=True,exist_ok=True)
settings=json.loads((take/'session.json').read_text());result=json.loads((take/'result.json').read_text());rows=list(csv.DictReader((take/'frames.csv').open()));presents=list(csv.DictReader((take/'presents.csv').open()))
for r in rows:
 for k in ['id','source_A','source_B','width','height','byte_offset','byte_count']:r[k]=int(r[k])
 for k in ['time_s','timestep']:r[k]=float(r[k])
for r in presents:
 for k in ['image_id','source_A','source_B']:r[k]=int(r[k])
 r['present_time_s']=float(r['present_time_s'])
streams={k:[r for r in rows if r['kind']==k] for k in 'RG'};lookup={(r['kind'],r['id']):r for r in rows}
maps={k:np.memmap(take/name,dtype=np.uint8,mode='r') for k,name in [('R','raw.bgra'),('G','generated.bgra')]}
def pixels(r):return maps[r['kind']][r['byte_offset']:r['byte_offset']+r['byte_count']].reshape(r['height'],r['width'],4)
def stats(v):
 if not v:return None
 return {'count':len(v),'mean':float(np.mean(v)),'median':float(np.median(v)),'p95':float(np.percentile(v,95)),'p99':float(np.percentile(v,99)),'max':float(max(v)),'min':float(min(v))}
integrity={}
for k,seq in streams.items():
 off=0
 for r in seq:
  assert r['byte_offset']==off;assert r['byte_count']==r['width']*r['height']*4;assert 0<=r['time_s']<result['duration_seconds'];off+=r['byte_count']
 assert off==maps[k].size
 integrity[k]={'frames':len(seq),'bytes':off,'contiguous_ids':all(b['id']==a['id']+1 for a,b in zip(seq,seq[1:]))}
hashes={};raw_changed=[];last=None
for r in streams['R']:
 digest=hashlib.sha256(pixels(r)[:,:,:3].tobytes()).hexdigest();hashes[('R',r['id'])]=digest
 if digest!=last:raw_changed.append(r)
 last=digest
print('Raw hashes complete',flush=True)
metrics=[];missing=[];copy_ids=[]
for index,r in enumerate(streams['G']):
 g=pixels(r);digest=hashlib.sha256(g[:,:,:3].tobytes()).hexdigest();hashes[('G',r['id'])]=digest
 a=lookup.get(('R',r['source_A']));b=lookup.get(('R',r['source_B']))
 if not a or not b:missing.append(r['id']);continue
 aa=pixels(a)[::4,::4,:3].astype(np.float32);bb=pixels(b)[::4,::4,:3].astype(np.float32);gg=g[::4,::4,:3].astype(np.float32)
 ca=digest==hashes['R',a['id']];cb=digest==hashes['R',b['id']]
 if ca or cb:copy_ids.append({'id':r['id'],'time_s':r['time_s'],'equals_A':ca,'equals_B':cb})
 m={'id':r['id'],'index':index,'time_s':r['time_s'],'A':a['id'],'B':b['id'],'source_gap_ms':1000*(b['time_s']-a['time_s']),'B_to_generated_available_ms':1000*(r['time_s']-b['time_s']),'exact_A':ca,'exact_B':cb,'motion_mae':float(np.mean(abs(aa-bb))),'generated_vs_A_mae':float(np.mean(abs(gg-aa))),'generated_vs_B_mae':float(np.mean(abs(gg-bb))),'generated_vs_linear_blend_mae':float(np.mean(abs(gg-(aa*(1-r['timestep'])+bb*r['timestep']))))}
 metrics.append(m)
 if index%100==0:print('Generated',index,flush=True)
changes=[];previous=None;first_present={};unknown=set()
for p in presents:
 key=(p['kind'].upper(),p['image_id']);first_present.setdefault(key,p['present_time_s'])
 if key not in lookup:unknown.add(key)
 if key!=previous:changes.append(p)
 previous=key
latency={'raw':[],'generated':[]};capture_gaps=[];ordering=[]
for p in changes:
 b=lookup.get(('R',p['source_B']))
 if b:latency['generated' if p['kind']=='g' else 'raw'].append(1000*(p['present_time_s']-b['time_s']))
for i,p in enumerate(changes[:-1]):
 nxt=changes[i+1]
 same_pair_next_g=nxt['kind']=='g' and nxt['source_A']==p['source_A'] and nxt['source_B']==p['source_B'] and float(nxt['timestep'])>float(p['timestep'])
 correct_b=nxt['kind']=='r' and nxt['image_id']==p['source_B']
 if p['kind']=='g' and not (same_pair_next_g or correct_b):ordering.append({'time':p['present_time_s'],'generated_id':p['image_id'],'next_kind':nxt['kind'],'next_id':nxt['image_id'],'expected_B':p['source_B']})
timings=[0]+[p['present_time_s'] for p in presents]+[result['duration_seconds']]
bins=[]
for sec in range(math.ceil(result['duration_seconds'])):
 bins.append({'second':sec,'raw':sum(sec<=r['time_s']<sec+1 for r in streams['R']),'raw_changes':sum(sec<=r['time_s']<sec+1 for r in raw_changed),'generated':sum(sec<=r['time_s']<sec+1 for r in streams['G']),'submitted_changes':sum(sec<=p['present_time_s']<sec+1 for p in changes)})
summary={'recording_id':take.name,'settings':settings,'result':result,'integrity':integrity,'raw_exact_adjacent_repeats':len(streams['R'])-len(raw_changed),'raw_distinct_hashes':len(set(h for (k,id),h in hashes.items() if k=='R')),'fully_comparable_generated':len(metrics),'generated_with_outside_window_sources':missing,'exact_endpoint_copies':copy_ids,'missing_present_references':sorted(unknown),'generated_never_submitted':[r['id'] for r in streams['G'] if ('G',r['id']) not in first_present],'ordering_anomalies':ordering,'source_pair_gap_ms':stats([m['source_gap_ms'] for m in metrics]),'generated_available_after_B_ms':stats([m['B_to_generated_available_ms'] for m in metrics]),'all_present_gap_ms':stats([1000*(b-a) for a,b in zip(timings,timings[1:])]),'content_submission_gap_ms':stats([1000*(b['present_time_s']-a['present_time_s']) for a,b in zip(changes,changes[1:])]),'capture_B_to_first_present_ms':{k:stats(v) for k,v in latency.items()},'bins':bins,'limitations':['No true source intermediate ground truth; pixel distances measure change, not accuracy.','Exact endpoint equality checks every RGB pixel; MAE uses a 4-pixel spatial stride.','Time begins at capture callback; excludes console rendering, capture hardware buffering and physical monitor scanout.','Present events are submissions, not observed monitor frames.','Diagnostic recording may perturb timing. First/last referenced images may lie outside the capture window.']}
(out/'audit.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
with (out/'all-generated-frames.csv').open('w',newline='') as f:
 writer=csv.DictWriter(f,fieldnames=metrics[0].keys());writer.writeheader();writer.writerows(metrics)
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',22);small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',18)
def im(r):return Image.fromarray(pixels(r)[:,:,2::-1].copy())
sheet=Image.new('RGB',(1600,5*330),'#0b1320');d=ImageDraw.Draw(sheet)
for sec in range(15):
 r=min(streams['R'],key=lambda r:abs(r['time_s']-sec));x=(sec%3)*533;y=(sec//3)*330;sheet.paste(im(r).resize((533,300)),(x,y+30));d.text((x+8,y+3),f'{sec}s | RAW #{r["id"]}',font=small,fill='white')
sheet.save(out/'source-timeline.jpg',quality=92)
selected=[]
for m in sorted(metrics,key=lambda m:m['motion_mae'],reverse=True):
 if all(abs(m['time_s']-x['time_s'])>.8 for x in selected):selected.append(m)
 if len(selected)==8:break
for id in [x['id'] for x in copy_ids[:3]]:
 m=next(m for m in metrics if m['id']==id)
 if m not in selected:selected.append(m)
selected.sort(key=lambda m:m['time_s'])
for m in selected:
 triplet=Image.new('RGB',(1920,405),'#0b1320');d=ImageDraw.Draw(triplet)
 for i,(r,label) in enumerate([(lookup['R',m['A']],'RAW A'),(lookup['G',m['id']],'GENERATED'),(lookup['R',m['B']],'RAW B')]):
  triplet.paste(im(r).resize((640,360)),(i*640,45));d.text((i*640+10,8),f'{label} #{r["id"]} | {m["time_s"]:.3f}s',font=small,fill='#55ddbb' if i==1 else 'white')
 triplet.save(out/f'pair-{m["id"]}.png')
chart=Image.new('RGB',(1400,500),'#0b1320');d=ImageDraw.Draw(chart);d.text((25,15),'Recorded frames and content submissions / second',font=font,fill='white')
for k,label,col in [('raw','Capture','#59daba'),('raw_changes','Exact image changes','#ffc36b'),('generated','VFG outputs','#ad99ff'),('submitted_changes','Submitted content','#85b6ff')]:
 pts=[(60+b['second']*90,420-b[k]*3.5) for b in bins];d.line(pts,fill=col,width=3);j=['raw','raw_changes','generated','submitted_changes'].index(k);d.text((50+j*330,455),label,font=small,fill=col)
for n in range(15):d.text((55+n*90,425),str(n),font=small,fill='white')
for n in [0,20,40,60,80,100]:d.text((10,410-n*3.5),str(n),font=small,fill='#b4bfd1')
chart.save(out/'cadence.png')
(out/'selected-pairs.json').write_text(json.dumps(selected,indent=2))
print(json.dumps(summary,indent=2),flush=True)
