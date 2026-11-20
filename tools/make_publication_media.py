"""Build explicitly mapped comparison assets from captured pixels, never recompute AI."""
from pathlib import Path
import sys,json,bisect,subprocess,hashlib,math
from study_publication import Take,ROOT
from PIL import Image,ImageDraw,ImageFont
from export_pipeline import subtitle_cues,srt_time

FFMPEG=next((ROOT/'deps/python/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
FONT=ImageFont.truetype(str(ROOT/'website/assets/fonts/font-2.ttf'),23)
SMALL=ImageFont.truetype(str(ROOT/'website/assets/fonts/font-0.ttf'),16)
def encoder(path,size,fps=60):
 return subprocess.Popen([str(FFMPEG),'-hide_banner','-loglevel','error','-f','rawvideo','-pix_fmt','rgb24','-s',f'{size[0]}x{size[1]}','-r',str(fps),'-i','pipe:0','-an','-c:v','libx264','-crf','17','-preset','fast','-threads','2','-pix_fmt','yuv420p','-movflags','+faststart','-y',str(path)],stdin=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
def finish(p):
 p.stdin.close()
 if p.wait():raise RuntimeError('Video encoder failed')
def panel(images,labels,foot='',width=960):
 height=round(images[0].height/images[0].width*width);height+=height%2
 out=Image.new('RGB',(width*len(images),height+76),'#10160f');d=ImageDraw.Draw(out)
 for i,(im,label) in enumerate(zip(images,labels)):
  out.paste(im.resize((width,height),Image.Resampling.LANCZOS),(i*width,42));d.text((i*width+14,9),label,font=FONT,fill='#b6ff59' if i else '#f1f1eb')
 d.text((14,height+50),foot,font=SMALL,fill='#aab5a3');return out
def gif(video,path):
 subprocess.run([str(FFMPEG),'-hide_banner','-loglevel','error','-i',str(video),'-filter_complex','fps=15,scale=960:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=192[p];[b][p]paletteuse=dither=bayer:bayer_scale=3','-threads','2','-loop','0','-y',str(path)],check=True)
def camera(source,audit,out,start=1.,length=3.):
 t=Take(source);raw=json.loads((audit/'raw-changes.json').read_text());pairs=json.loads((audit/'generated-pairs.json').read_text());rt=[r['time_s'] for r in raw]
 fg=sorted([(r['time_s'],('R',r['id'])) for r in raw]+[(g['source_mid_s'],('G',g['id'])) for g in pairs]);ft=[x[0] for x in fg];mapping=[]
 real=encoder(out/'gta-2x-realtime.mp4',(1920,616),60);slow=encoder(out/'gta-2x-slow.mp4',(1920,616),15);sample=encoder(out/'gta-source.mp4',(1280,720),30)
 for n in range(round(length*60)):
  ts=start+n/60;ri=max(0,bisect.bisect_right(rt,ts)-1);fi=max(0,bisect.bisect_right(ft,ts)-1);rk=('R',raw[ri]['id']);fk=fg[fi][1];a=t.image(rk);b=t.image(fk)
  im=panel([a,b],['ORIGINAL INPUT','NVIDIA VFG 2x'],f'Source aligned / source {ts:.3f}s / RAW {rk[1]} / {fk[0]} {fk[1]} / no enhancer');data=im.tobytes();real.stdin.write(data);slow.stdin.write(data)
  if n%2==0:sample.stdin.write(a.resize((1280,720),Image.Resampling.LANCZOS).tobytes())
  mapping.append({'frame':n,'source_time_s':ts,'left':list(rk),'right':list(fk)})
 for p in (real,slow,sample):finish(p)
 selected=[p for p in pairs if start<=p['source_mid_s']<start+length]
 chosen=next((p for p in selected if 1.9<p['source_mid_s']<2),selected[len(selected)//2]);ims=[t.image(('R',chosen['a'])),t.image(('G',chosen['id'])),t.image(('R',chosen['b']))]
 panel(ims,['RAW A','GENERATED midpoint','RAW B'],f'G {chosen["id"]} / full source 2560 x 1440 / same input pair',width=640).save(out/'gta-fg-pair.png')
 for im,name in zip(ims,['a','generated','b']):im.save(out/f'gta-fg-{name}-full.png')
 # Identical native-pixel crops, no sharpen/upscale.
 crop=(1000,300,1540,950);panel([im.crop(crop) for im in ims],['RAW A','GENERATED','RAW B'],'Native-pixel crop / coordinates '+str(crop),width=540).save(out/'gta-fg-crop.png')
 meta={'recording_id':source.name,'start_s':start,'length_s':length,'playback_slowdown':4,'output_fps':{'realtime':60,'slow':15},'mapping':mapping,'selected_pair':chosen,'input_changed_frames_in_window':sum(start<=r['time_s']<start+length for r in raw),'generated_frames_in_window':len(selected),'endpoint_copies':sum(p['copy_a'] or p['copy_b'] for p in selected),'note':'Only recorded frames. Source-time alignment removes processing delay. Encoded at 60 FPS does not prove 60 scanout FPS; source timestamps and frame holds are retained.'}
 (out/'gta-2x-mapping.json').write_text(json.dumps(meta,indent=2));gif(out/'gta-2x-slow.mp4',out/'gta-2x-slow.gif');t.close();print('camera ready',flush=True)

def paired(source,audit,out,name,preview_start=0,preview_length=None):
 t=Take(source);report=json.loads((audit/'audit.json').read_text());rows=json.loads((audit/'valid-outputs.json').read_text());end=report['usable_prefix_end_s'];rows=[r for r in rows if r['time_s']<end];times=[r['time_s'] for r in rows]
 first=times[0];start=max(first,preview_start);length=min(end-start,preview_length or end);fps=60
 pair=encoder(out/f'{name}.mp4',(1920,616),fps);final=encoder(out/f'{name}-final.mp4',(1920,1080),fps);before_video=encoder(out/f'{name}-before.mp4',(1920,1080),fps);after_video=encoder(out/f'{name}-after.mp4',(1920,1080),fps)
 mappings=[]
 for n in range(max(1,int(length*fps))):
  time=start+n/fps;r=rows[max(0,bisect.bisect_right(times,time)-1)];before=t.image(r['before']);after=t.image(r['after']);fin=t.image(r['final']);
  has_sub=bool(report['subtitle_cues']);right=fin if has_sub else after
  im=panel([before,right],['BEFORE ENHANCER','FINAL + CZECH SUBTITLES' if has_sub else 'AFTER ENHANCER'],f'{source.name} / {time:.3f}s / matching output {r["output_id"]} / recorded pixels');pair.stdin.write(im.tobytes());before_video.stdin.write(before.resize((1920,1080),Image.Resampling.LANCZOS).tobytes());after_video.stdin.write(after.resize((1920,1080),Image.Resampling.LANCZOS).tobytes());final.stdin.write(fin.resize((1920,1080),Image.Resampling.LANCZOS).tobytes());mappings.append({'frame':n,'recording_time_s':time,'output_id':r['output_id']})
 finish(pair);finish(final);finish(before_video);finish(after_video)
 choices=[start+length*.18,start+length*.5,start+length*.8]
 for i,time in enumerate(choices):
  r=rows[max(0,bisect.bisect_right(times,time)-1)];before=t.image(r['before']);after=t.image(r['after']);fin=t.image(r['final'])
  panel([before,after],['BEFORE ENHANCER','AFTER ENHANCER'],f'{source.name} / output {r["output_id"]} / same input / strength {r.get("nr_strength",1)*100:.0f}%').save(out/f'{name}-pair-{i+1}.png')
  if i==1:
   before.save(out/f'{name}-before-full.png');after.save(out/f'{name}-after-full.png');fin.save(out/f'{name}-final-full.png')
   crop=(940,320,1580,960);panel([before.crop(crop),after.crop(crop)],['BEFORE','AFTER'],'Native-pixel crop / '+str(crop),width=640).save(out/f'{name}-crop.png')
 cues=subtitle_cues(rows,end)
 (out/f'{name}.srt').write_text(''.join(f'{i+1}\n{srt_time(max(0,c["start_s"]-start))} --> {srt_time(min(length,c["end_s"]-start))}\n{c["text"]}\n\n' for i,c in enumerate(cues) if c['end_s']>start and c['start_s']<start+length),encoding='utf-8-sig')
 (out/f'{name}-mapping.json').write_text(json.dumps({'recording_id':source.name,'exported_from_s':start,'exported_to_s':start+length,'original_result':t.result,'missing_output_rows':report['missing_output_rows'],'subtitles':cues,'mapping':mappings},ensure_ascii=False,indent=2),encoding='utf-8');t.close();print(name+' ready',flush=True)

def main():
 source=Path(sys.argv[1]);audit=Path(sys.argv[2]);out=Path(sys.argv[3]);out.mkdir(parents=True,exist_ok=True);mode=sys.argv[4] if len(sys.argv)>4 else 'all'
 if mode in ('all','camera'):camera(source/'analysis-15s-112499968',audit/'analysis-15s-112499968',out)
 if mode in ('all','pairs'):
  for rid,name in [('114426140','gta-enhancer-1'),('114773125','gta-enhancer-2'),('114955078','gta-subtitles-1'),('114991406','gta-subtitles-2'),('115015921','gta-subtitles-3')]:paired(source/f'analysis-15s-{rid}',audit/f'analysis-15s-{rid}',out,name)

if __name__=='__main__':main()
