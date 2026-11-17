from pathlib import Path
import csv,json,subprocess,sys,hashlib
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'deps/python'))
from PIL import Image,ImageDraw,ImageFont,ImageChops
folder=ROOT/'app/benchmarks/csgo';ffmpeg=next((ROOT/'deps/python/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
rows=list(csv.DictReader((folder/'summary.csv').open()))
if len(rows)!=9 or not (folder/'complete.txt').exists():raise RuntimeError('Benchmark incomplete')
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',23);small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',18)
canvas=Image.new('RGB',(1400,790),'#0c1420');d=ImageDraw.Draw(canvas)
d.text((28,20),'CS:GO | 1080p | withheld real frames | RTX 5070 Ti',font=font,fill='white')
d.text((28,62),'2x: 30 > 60 fps     3x: 20 > 60 fps     4x: 15 > 60 fps',font=small,fill='#99abbf')
for x,text in [(30,'Mode'),(230,'RGB PSNR (higher = closer)'),(740,'VFG GPU ms / output (lower = faster)'),(1250,'Copies')]:d.text((x,110),text,font=small,fill='white')
for i,r in enumerate(rows):
 y=156+i*57;ps=float(r['mean_psnr_db']);gpu=float(r['gpu_mean_ms']);color={'low':'#88b9ff','medium':'#44ddbb','high':'#ffb96b'}[r['quality']]
 d.text((30,y),r['quality']+' '+r['multiplier']+'x',font=font,fill=color)
 d.rectangle((230,y+5,230+ps*8,y+30),fill=color);d.text((635,y),f'{ps:.2f}',font=small,fill='white')
 d.rectangle((740,y+5,740+gpu*27,y+30),fill=color);d.text((1160,y),f'{gpu:.2f}',font=small,fill='white');d.text((1270,y),r['exact_endpoint_copies'],font=small,fill='white')
d.text((28,705),'GPU includes VFG + output copy; excludes upload, readback, disk, monitor. No end-to-end latency claim.',font=small,fill='#99abbf')
d.text((28,740),'PSNR is a pixel error metric, not a perceptual quality verdict. Larger multipliers have sparser inputs.',font=small,fill='#99abbf')
canvas.save(folder/'results.png')
for m in [2,3,4]:
 subprocess.run([str(ffmpeg),'-hide_banner','-loglevel','error','-f','rawvideo','-pixel_format','bgra','-video_size','1920x1080','-framerate','60','-i',str(folder/f'medium-{m}x.bgra'),'-c:v','libx264','-preset','fast','-crf','16','-pix_fmt','yuv420p','-movflags','+faststart','-y',str(folder/f'medium-{m}x.mp4')],check=True)
 truth=Image.open(folder/f'medium-{m}x-worst-truth.bmp').convert('RGB');gen=Image.open(folder/f'medium-{m}x-worst-generated.bmp').convert('RGB')
 strip=Image.new('RGB',(1920,580),'#0c1420');sd=ImageDraw.Draw(strip)
 for i,(im,label) in enumerate([(truth,'REAL withheld frame'),(gen,f'NVIDIA Medium {m}x'),(ImageChops.multiply(ImageChops.difference(truth,gen),Image.new('RGB',truth.size,(255,255,255))),'Absolute RGB difference')]):
  strip.paste(im.resize((640,360)),(i*640,45));sd.text((i*640+10,8),label,font=small,fill='white')
 # Native resolution center crop preserves fine edge differences.
 for i,im in enumerate([truth,gen,ImageChops.difference(truth,gen)]):strip.paste(im.crop((640,450,1280,620)),(i*640,410))
 strip.save(folder/f'medium-{m}x-worst.png')
cmd=[str(ffmpeg),'-hide_banner','-loglevel','error','-i',str(ROOT/'app/samples/csgo-reference.mkv')]
for m in [2,3,4]:cmd+=['-i',str(folder/f'medium-{m}x.mp4')]
cmd+=['-filter_complex','[0:v]scale=640:360[a];[1:v]scale=640:360[b];[2:v]scale=640:360[c];[3:v]scale=640:360[d];[a][b][c][d]hstack=inputs=4:shortest=1[out]','-map','[out]','-an','-c:v','libx264','-crf','16','-pix_fmt','yuv420p','-movflags','+faststart','-y',str(folder/'reference-2x-3x-4x.mp4')];subprocess.run(cmd,check=True)
parts=['<!doctype html><meta charset="utf-8"><title>FrameLab benchmark</title><style>body{background:#0c1420;color:#edf5ff;font:18px system-ui;max-width:1400px;margin:30px auto}img,video{max-width:100%}td,th{padding:10px;text-align:left}a{color:#44ddbb}</style><h1>CS:GO — měření NVIDIA VFG</h1><p>Čtyři sekundy 1080p60 z úseku 25–29 s. Vynechané snímky jsou skutečná reference. Testy 2× / 3× / 4× obnovují 30 / 20 / 15 fps zpět na 60 fps. Nejde o srovnání stejného vstupního FPS.</p><img src="results.png"><h2>Reference | Medium 2× | Medium 3× | Medium 4×</h2><video controls loop src="reference-2x-3x-4x.mp4"></video><p>Video je náhled s kompresí H.264; měření používá původní BGRA pixely. Časy GPU nejsou latence střihovky ani monitoru.</p><table><tr><th>Režim</th><th>NVIDIA PSNR</th><th>Opakování A</th><th>Prolnutí A/B</th><th>GPU P95 ms</th></tr>']
for r in rows:parts.append('<tr>'+''.join(f'<td>{v}</td>' for v in [r['quality']+' '+r['multiplier']+'×',r['mean_psnr_db'],r['mean_hold_psnr_db'],r['mean_blend_psnr_db'],r['gpu_p95_ms']])+'</tr>')
parts.append('</table>')
for m in [2,3,4]:parts.append(f'<h2>Medium {m}× — snímek s nejnižším PSNR</h2><img src="medium-{m}x-worst.png">')
parts.append('<p><a href="summary.csv">CSV výsledky</a> · <a href="../../samples/provenance.json">Původ vzorku</a></p>')
(folder/'report.html').write_text('\n'.join(parts),encoding='utf-8')
print(json.dumps(rows,indent=2))
