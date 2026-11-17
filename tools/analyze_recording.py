import sys, pathlib, csv, json, statistics, subprocess
ROOT=pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT/'deps/python'))
from PIL import Image, ImageChops, ImageStat, ImageDraw, ImageFont
take=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else max((ROOT/'app/recordings').glob('take-*'),key=lambda p:p.stat().st_mtime)
out=ROOT/'analysis-output'/take.name
out.mkdir(parents=True,exist_ok=True)
rows=list(csv.DictReader((take/'pairs.csv').open()))
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',24)
small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',19)
def load(n,k): return Image.open(take/f'{n:04d}-{k}.bmp').convert('RGB')
def mae(a,b): return sum(ImageStat.Stat(ImageChops.difference(a,b)).mean)/3
metrics=[]
for n,r in enumerate(rows):
 a,m,b=[load(n,k) for k in 'AMB']
 aa,mm,bb=[im.resize((640,360)) for im in (a,m,b)]
 metrics.append({'index':n,'source_A':int(r['source_A']),'source_B':int(r['source_B']),'gap_ms':float(r['source_gap_ms']), 'motion_mae':mae(aa,bb),'vfg_vs_A_mae':mae(mm,aa),'vfg_vs_B_mae':mae(mm,bb),'vfg_vs_blend_mae':mae(mm,Image.blend(aa,bb,.5))})
chosen=sorted(set([0,len(rows)//3,2*len(rows)//3,len(rows)-1]+[r['index'] for r in sorted(metrics,key=lambda r:r['motion_mae'],reverse=True)[:2]]))
sheet=Image.new('RGB',(1536,310*len(chosen)),(15,20,28));draw=ImageDraw.Draw(sheet)
for j,n in enumerate(chosen):
 for i,k in enumerate('AMB'):
  im=load(n,k);im.thumbnail((512,288));sheet.paste(im,(i*512,j*310+22));draw.text((i*512+8,j*310),f'{n:02d} / '+{'A':'RAW A','M':'NVIDIA GENERATED','B':'RAW B'}[k],font=small,fill=(90,240,185) if k=='M' else 'white')
sheet.save(out/'overview.jpg',quality=94)
sequence=out/'triplets';sequence.mkdir(exist_ok=True)
for n,r in enumerate(rows):
 canvas=Image.new('RGB',(1920,406),(15,20,28));d=ImageDraw.Draw(canvas)
 for i,k in enumerate('AMB'):
  im=load(n,k).resize((640,360),Image.Resampling.LANCZOS);canvas.paste(im,(i*640,46));d.text((i*640+14,7),f'{n:02d}  '+{'A':'RAW A / before','M':'NVIDIA / midpoint only','B':'RAW B / after'}[k],font=font,fill=(90,240,185) if k=='M' else 'white')
 canvas.save(sequence/f'{n:04d}.png')
ffmpeg=next((ROOT/'deps/python/imageio_ffmpeg/binaries').glob('*.exe'))
subprocess.run([str(ffmpeg),'-hide_banner','-loglevel','error','-y','-framerate','10','-i',str(sequence/'%04d.png'),'-c:v','libx264','-crf','16','-pix_fmt','yuv420p','-movflags','+faststart',str(out/'comparison-slow.mp4')],check=True)
times=[float(r['source_B_time_s']) for r in rows]
summary={'take':str(take),'pairs':len(rows),'source_span_seconds':times[-1]-times[0], 'source_gap_ms_min':min(m['gap_ms'] for m in metrics),'source_gap_ms_max':max(m['gap_ms'] for m in metrics),'noncontiguous_pairs':sum(int(rows[i]['source_A'])!=int(rows[i-1]['source_B']) for i in range(1,len(rows))), 'chosen_overview_indices':chosen,'metrics':metrics,'limitations':'Actual matched live VFG triplets; GPU readback can disturb capture timing. Video shows pair inspections at 10 pairs/s, not the native display cadence. No ground-truth middle image. MAE is computed after 640x360 downsample and is not a quality score.'}
(out/'analysis.json').write_text(json.dumps(summary,indent=2))
print(json.dumps({k:v for k,v in summary.items() if k!='metrics'},indent=2))
print(out)
